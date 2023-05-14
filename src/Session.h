#ifndef __SESSION_H__
#define __SESSION_H__
#include <string>
#include "AsyncIO.h"
#include "IHandle.h"
#include <map>
#define MAX_BUF_SIZE 1024*10
extern std::map<int, IProtocolHandle*> g_mapProtocolHandle;
class CSession
{
public:
	CSession(CAsyncIO* pAsyncIO, int iSocket);
	void InsertMechanism(std::string& str);
	std::string& GetMechanism();
	void SetAuthStatus(int iStatus);
	int GetAuthStatus();
public:
	void recvCallBack(int nErrorCode, int nNumberOfBytes);
private:
	int m_authStatus;
	CAsyncIO* m_pAsyncIO;
	char* m_recvBuf;
	int m_hasRecvLen;
	int m_recvBufLen;
	char m_sendBuf[MAX_BUF_SIZE];
	size_t m_socket;
	std::string m_mechanism;
};

#endif // __SESSION_H__
