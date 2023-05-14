#include "SaslHandshakeHandle.h"
#include "Session.h"
#include "utils.h"
#include <vector>
#include <map>

CSaslHandshakeHandle::CSaslHandshakeHandle()
{
	m_minVersion = 0;
	m_maxVersion = 1;
	m_apiKey = 17;
}
int CSaslHandshakeHandle::minVersion() const
{
	return m_minVersion;
}
int CSaslHandshakeHandle::maxVersion() const
{
	return m_maxVersion;
}
int CSaslHandshakeHandle::key() const
{
	return m_apiKey;
}
int CSaslHandshakeHandle::handle(RequestHeader* pHeader, char* readBuf, char* writeBuf, void* pUsrData)
{
	char* ptr = readBuf;
	int len = readShort(ptr);
	std::string mechanism = readString(ptr, len);
	((CSession*)pUsrData)->InsertMechanism(mechanism);
	ptr = writeBuf + 4;
	writeInt(pHeader->correlation, ptr);	//correlation ID
	writeShort(ErrorCode_None, ptr);		//error no
	writeInt(1, ptr);						//count
	writeShort(mechanism.size(), ptr);
	writeString(mechanism.c_str(), mechanism.size(), ptr);
	int length = ptr - writeBuf;
	*(int*)writeBuf = htonl(length - 4);
	return length;
}