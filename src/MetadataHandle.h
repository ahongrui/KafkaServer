#ifndef  __METADATA_HANDLE_H__
#define  __METADATA_HANDLE_H__
#include "IHandle.h"
#define SERV_PORT 19092
class CMetadataHandle :public IProtocolHandle
{
public:
	CMetadataHandle();
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
