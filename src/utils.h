#ifndef __UTILS_H__
#define __UTILS_H__
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
void writeInt(int value, char* &ptr);
void writeShort(short value, char* &ptr);
void writeString(const char* str, int size, char* &ptr);
void writeChar(char ch, char* &ptr);
int readInt(char* &ptr);
short readShort(char* &ptr);
std::string readString(char* &ptr, int len);
int encI64ToVarint(char* dst, short dstSize, int64_t value);
int decVarintToI64(const char* src, short srcSize, int64_t& result);
int setBlock(int fd, bool bBlock);
int sendAndWait(int iSocketfd, char* buf, int iLen, unsigned int nTimeout);
#endif