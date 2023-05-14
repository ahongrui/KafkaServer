#include "Session.h"
#include <string>
#include "AsyncIO.h"
#include <map>
#include "utils.h"
#define MAX_BUF_SIZE 1024*10
CSession::CSession(CAsyncIO* pAsyncIO, int iSocket)
{
	m_authStatus = 0;
	m_pAsyncIO = pAsyncIO;
	m_socket = iSocket;
	m_hasRecvLen = 0;
	m_recvBufLen = MAX_BUF_SIZE;
	m_recvBuf = (char*)malloc(m_recvBufLen);
	int iRet = pAsyncIO->bindSocket(iSocket);
	if (iRet != 0)
	{
		printf("asyncIO bindSocket failed, fd:%d\n", iSocket);
		return;
	}
	pAsyncIO->asyncRecv(iSocket, m_recvBuf, m_recvBufLen, 
		std::bind(&CSession::recvCallBack, this, std::placeholders::_1, std::placeholders::_2));

}
void CSession::InsertMechanism(std::string& str)
{
	m_mechanism = str;
}
std::string& CSession::GetMechanism()
{
	return m_mechanism;
}
void CSession::SetAuthStatus(int iStatus)
{
	m_authStatus = iStatus;
}
int CSession::GetAuthStatus()
{
	return m_authStatus;
}
void CSession::recvCallBack(int nErrorCode, int nNumberOfBytes)
{
	if (nNumberOfBytes <= 0 && errno != EAGAIN && errno != EWOULDBLOCK)
	{
		printf("sessionID:%lu, the other side has been closed, errno:%d \n", m_socket, errno);
		m_pAsyncIO->unbindSocket(m_socket);
		::close(m_socket);
		return;
	}
	m_hasRecvLen += nNumberOfBytes;
	if (m_hasRecvLen < (int)sizeof(int))
	{
		//不够4字节int, 继续接收
		char* buf = m_recvBuf + m_hasRecvLen;
		int byteToRecv = m_recvBufLen - m_hasRecvLen;
		m_pAsyncIO->asyncRecv(m_socket, buf, byteToRecv,
			std::bind(&CSession::recvCallBack, this, std::placeholders::_1, std::placeholders::_2));
		return;
	}
	char* readBuf = m_recvBuf;
	int allSize = readInt(readBuf) + sizeof(int);
	if (allSize > m_recvBufLen) // 内存不够，重新申请
	{
		m_recvBufLen = allSize;
		m_recvBuf = (char*)realloc(m_recvBuf, allSize);
	}
	if (m_hasRecvLen < allSize)
	{
		//整包没收完成， 继续收
		char* buf = m_recvBuf + m_hasRecvLen;
		int byteToRecv = m_recvBufLen - m_hasRecvLen;
		m_pAsyncIO->asyncRecv(m_socket, buf, byteToRecv,
			std::bind(&CSession::recvCallBack, this, std::placeholders::_1, std::placeholders::_2));
		return;
	}
	else
	{
		//读取头
		RequestHeader* pHead = (RequestHeader*)readBuf;
		readBuf += 10;
		pHead->key = ntohs(pHead->key);
		std::map<int, IProtocolHandle*>::iterator it = g_mapProtocolHandle.find(pHead->key);
		if (it == g_mapProtocolHandle.end())
		{
			printf("sessionID:%lu unsupport apiKey:%d. \n", m_socket, pHead->key);
			memmove(m_recvBuf, m_recvBuf + allSize, m_hasRecvLen - allSize);
			m_hasRecvLen -= allSize;
			char* buf = m_recvBuf + m_hasRecvLen;
			int byteToRecv = m_recvBufLen - m_hasRecvLen;
			m_pAsyncIO->asyncRecv(m_socket, buf, byteToRecv,
				std::bind(&CSession::recvCallBack, this, std::placeholders::_1, std::placeholders::_2));
			return;
		}
		pHead->version = ntohs(pHead->version);
		pHead->correlation = ntohl(pHead->correlation);
		pHead->client_id_size = ntohs(pHead->client_id_size);
		if (pHead->client_id_size > 0)
		{
			readBuf += pHead->client_id_size; // client_id
		}
		printf("sessionID:%lu key:%d, version:%d, correlation:%d\n", m_socket, pHead->key, pHead->version, pHead->correlation);
		memset(m_sendBuf, 0, sizeof(m_sendBuf));
		if (pHead->version > it->second->maxVersion() || pHead->version < it->second->minVersion())
		{
			char* ptr = m_sendBuf + 4;
			writeInt(pHead->correlation, ptr);
			writeShort(ErrorCode_UnsupportedVersion, ptr);
			writeInt(0, ptr);

			int length = ptr - m_sendBuf;
			*(int*)(m_sendBuf) = htonl(length - 4);
			sendAndWait(m_socket, m_sendBuf, length, -1);

			memmove(m_recvBuf, m_recvBuf + allSize, m_hasRecvLen - allSize);
			m_hasRecvLen -= allSize;
			char* buf = m_recvBuf + m_hasRecvLen;
			int byteToRecv = m_recvBufLen - m_hasRecvLen;
			m_pAsyncIO->asyncRecv(m_socket, buf, byteToRecv,
				std::bind(&CSession::recvCallBack, this, std::placeholders::_1, std::placeholders::_2));
			return;
		}
		int length = it->second->handle(pHead, readBuf, m_sendBuf, this);
		sendAndWait(m_socket, m_sendBuf, length, -1);
		memmove(m_recvBuf, m_recvBuf + allSize, m_hasRecvLen - allSize);
		m_hasRecvLen -= allSize;
		char* buf = m_recvBuf + m_hasRecvLen;
		int byteToRecv = m_recvBufLen - m_hasRecvLen;
		m_pAsyncIO->asyncRecv(m_socket, buf, byteToRecv,
			std::bind(&CSession::recvCallBack, this, std::placeholders::_1, std::placeholders::_2));
		return;
	}
}

