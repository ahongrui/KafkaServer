#ifndef  __SASL_HANDSHAKE_HANDLE_H__
#define  __SASL_HANDSHAKE_HANDLE_H__
#include "IHandle.h"
class CSaslHandshakeHandle :public IProtocolHandle
{
public:
	CSaslHandshakeHandle();
	virtual int minVersion() const;
	virtual int maxVersion() const;
	virtual int key() const;
	virtual int handle(RequestHeader* pHeader, char* readBuf, char* writeBuf, void* pUsrData);
private:
	int m_minVersion;
	int m_maxVersion;
	int m_apiKey;
};

#endif
