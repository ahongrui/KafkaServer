#ifndef  __SASL_AUTHENTICATE_HANDLE_H__
#define  __SASL_AUTHENTICATE_HANDLE_H__
#include "IHandle.h"
#include "gssapi.h"
class CSaslAuthenticateHandle :public IProtocolHandle
{
public:
	CSaslAuthenticateHandle();
	virtual int minVersion() const;
	virtual int maxVersion() const;
	virtual int key() const;
	virtual int handle(RequestHeader* pHeader, char* readBuf, char* writeBuf, void* pUsrData);
private:
	void init();
private:
	int m_minVersion;
	int m_maxVersion;
	int m_apiKey;
	std::string m_servicePrimary;
	std::string m_keytab;
	gss_cred_id_t m_serverCreds;
};


#endif
