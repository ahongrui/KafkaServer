#include "InitProducerHandle.h"
#include "utils.h"
#include <vector>

CInitProducerHandle::CInitProducerHandle()
{
	m_minVersion = 0;
	m_maxVersion = 1;
	m_apiKey = 22;
}
int CInitProducerHandle::minVersion() const
{
	return m_minVersion;
}
int CInitProducerHandle::maxVersion() const
{
	return m_maxVersion;
}
int CInitProducerHandle::key() const
{
	return m_apiKey;
}
int CInitProducerHandle::handle(RequestHeader* pHeader, char* readBuf, char* writeBuf, void* pUsrData)
{
	char* ptr = writeBuf + 4;
	writeInt(pHeader->correlation, ptr);
	writeInt(0, ptr); //Throtlle time Á÷¿Ø
	writeShort(0, ptr);
	writeInt(0, ptr); // Proudcer ID
	writeInt(17001, ptr);
	writeShort(0, ptr); //producer epoch

	int length = ptr - writeBuf;
	*(int*)writeBuf = htonl(length - 4);
	return length;
}