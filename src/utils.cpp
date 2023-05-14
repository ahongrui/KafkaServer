#include "utils.h"

void writeInt(int value, char* &ptr)
{
	*(int*)ptr = htonl(value);
	ptr += sizeof(int);
}
void writeShort(short value, char* &ptr)
{
	*(short*)ptr = htons(value);
	ptr += sizeof(short);
}
void writeString(const char* str, int size, char* &ptr)
{
	memcpy(ptr, str, size);
	ptr += size;
}
void writeChar(char ch, char* &ptr)
{
	*(char*)ptr = ch;
	ptr += sizeof(char);
}
int readInt(char* &ptr)
{
	int value = ntohl(*(int*)ptr);
	ptr += sizeof(int);
	return value;
}
short readShort(char* &ptr)
{
	short value = ntohs(*(short*)ptr);
	ptr += sizeof(short);
	return value;
}
std::string readString(char* &ptr, int len)
{
	char* p = ptr;
	ptr += len;
	return std::string(p, len);
}
int encI64ToVarint(char* dst, short dstSize, int64_t value)
{
	unsigned long long tmp = (value << 1) ^ (value >> 63);//zig-zag encoding
	short idx = 0;
	do {
		if (idx >= value)
		{
			return 0; // dst space not enough
		}
		dst[idx++] = (tmp & 0x7f) | (tmp > 0x7f ? 0x80 : 0);
		tmp >> 7;
	} while (tmp);
	return idx;
}
int decVarintToI64(const char* src, short srcSize, int64_t& result)
{
	long long num = 0;
	short idx = 0;
	short shift = 0;
	do 
	{
		if (srcSize-- == 0)
		{
			return 0; //varint 最后一位首字节为0
		}
		num |= (long long)(src[idx] & 0x7f) << shift;
		shift += 7;
	} while(src[idx++] & 0x80);
	result = (long long)(num >> 1) ^ -(long long)(num & 1);//zig-zag decoding
	return idx;
}

int setBlock(int fd, bool bBlock)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (bBlock)
	{
		return fcntl(fd, F_SETFL, flags &(~O_NONBLOCK));
	}
	else
	{
		return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}
}

int sendAndWait(int iSocketfd, char* buf, int iLen, unsigned int nTimeout)
{
	struct pollfd fds[1] = { 0 };
	fds[0].fd = iSocketfd;
	fds[0].events = POLLWRNORM;
	int iRet = poll(fds, 1, nTimeout);
	if (iRet > 0 && (fds[0].revents & POLLWRNORM))
	{
		printf("send:%d\n",iLen);
		return send(iSocketfd, buf, iLen, MSG_NOSIGNAL);
	}
	else
	{
		return -1;
	}
}
