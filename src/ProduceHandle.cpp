#include "ProduceHandle.h"
#include "utils.h"
#include <vector>
#include <stdio.h>
#include <string.h>
#include <map>

CProduceHandle::CProduceHandle()
{
	m_minVersion = 0;
	m_maxVersion = 7;
	m_apiKey = 0;
}
int CProduceHandle::minVersion() const
{
	return m_minVersion;
}
int CProduceHandle::maxVersion() const
{
	return m_maxVersion;
}
int CProduceHandle::key() const
{
	return m_apiKey;
}
int CProduceHandle::handle(RequestHeader* pHeader, char* readBuf, char* writeBuf, void* pUsrData)
{
	//MessageRequest: Transaction ID(int16) + Required Acks(int16) + Timeout(int32) + topic count(int32) + [topic{}]
	//topic{}: topic name(varint+str) + partion count(int32) + [partition{}]
	//partition{}: partition ID(int32) + Message Set len(int32) + offset(int64) + Message Size(int32) + Leader Epoch(int32)
	//				+ Magic Byte(int8) + CRC32(int32) + Attributes(int16,低2位- 0 none 1 Gzip 2 Snappy) + Last Offset Delta(int32)
	//				+ First Timestamp(int64) + Last Timestamp(int64) + Producer ID(int64) + Producer Epoch(int16) + Base sequence(int32)
	//				+Record Count(int32) + [Record{}]
	//Record{}:record length(varint) + attributes(int8) + Timestamp(varint) + Offset(varint) + Key Lenth(varint) + Key + value len(varint)
	//				+ Headers{}
	//Headers{}: HeaderKey Len(varint) + HeaderKey + HeadValue Len(varint) + HeadValue
	std::map<std::string, std::vector<int> > mapTopicPartition;
	char* ptr = readBuf + 2;//Transaction ID
	bool bNeedRespond = readShort(ptr) & 0x3; //0-无需应答 1、-1需要应答
	ptr += 4; //Timeout
	int topicNum = readInt(ptr);//topic count
	for (int i = 0; i < topicNum; ++i)
	{
		short len = readShort(ptr); //string length
		std::string topicName = readString(ptr, len); //topic name
		int partitionNum = readInt(ptr); //Partition count
		for (int j = 0; j < partitionNum; ++j)
		{
			int partitionId = readInt(ptr);
			mapTopicPartition[topicName].push_back(partitionId);
			ptr += 4 //message set length
				+ 8 //offset
				+ 4 //message size
				+ 4 //leader epoch
				+ 1 //magic byte
				+ 4	//crc32
				+ 2 //attributes
				+ 4 //last offset delta
				+ 8 //first timestamp
				+ 8 //last timestamp
				+ 8 //producer ID
				+ 2 //producer epoch
				+ 4; //base sequence 
			int recordCount = readInt(ptr);
			for (int k = 0; k < recordCount; ++k)
			{
				int64_t tmpNum = 0;
				ptr += decVarintToI64(ptr, 8, tmpNum); //record length
				ptr += 1; //attributes
				ptr += decVarintToI64(ptr, 8, tmpNum); //Timestamp
				ptr += decVarintToI64(ptr, 8, tmpNum); //offset
				ptr += decVarintToI64(ptr, 8, tmpNum); //key length
				if (tmpNum > 0) ptr += tmpNum; //key
				ptr += decVarintToI64(ptr, 8, tmpNum); //value length
				std::string value = readString(ptr, tmpNum);//value
				printf("produce: %s\n", value.c_str());
			}
		}
	}
	if (bNeedRespond)
	{
		//MessageReponse: length(int32) + correlation(int32) + topic count(int32) + [topic{}] + throttle time(int32)
		//topic{}: topic name(int16 + str) + partition count(int32) + [partition{}]
		//partition{}: partition ID(int32) + error(int16) + offset(int64) + time(int64) + log start offset(int64)
		ptr = writeBuf + 4; // 前4字节表示总长度
		writeInt(pHeader->correlation, ptr); //correlation
		writeInt(mapTopicPartition.size(), ptr); //topic count
		std::map<std::string, std::vector<int> >::iterator it = mapTopicPartition.begin();
		for (; it != mapTopicPartition.end(); ++it)
		{
			short len = it->first.size();
			writeShort(len, ptr); //topic name length
			writeString(it->first.c_str(), len, ptr);//topic name
			std::vector<int>& vecPatition = it->second;
			int partitionNum = vecPatition.size();
			writeInt(partitionNum, ptr); //partition count
			for (int i = 0; i < partitionNum; ++i)
			{
				writeInt(vecPatition[i], ptr); // partition ID
				writeShort(ErrorCode_None, ptr); //error
				memset(ptr, 0, 8);//offset
				ptr += 8;
				memset(ptr, 0xff, 8);//time
				ptr += 8;
				memset(ptr, 0, 8);//log start offset
				ptr += 8;
			}
		}
		writeInt(0, ptr); //throttle time
		int length = ptr - writeBuf;
		*(int*)writeBuf = htonl(length - 4);
		return length;
	}
	return 0;
}