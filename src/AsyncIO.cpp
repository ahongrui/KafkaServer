#include "AsyncIO.h"
#include "utils.h"
#include <unistd.h>
#include <error.h>

#define MAX_RECV_QUEUE  (128*30)
#define MAX_SEND_BUFFER (32*1024*1024) //发送缓存不超过32M
#define MAX_SEND_LENGTH (16*24) //每次发送数据不超过16k
CSocketOperation::CSocketOperation(int socketFd, int epollFd)
{
	m_socketFd = socketFd;
	m_epollFd = epollFd;
	m_mallocSize = 0;
	m_events = 0;
	m_memLimitSize = MAX_SEND_BUFFER;
	m_sendQueue.clear();
	m_recvQueue.clear();
	changeEpollOpt(OPERATION_ADD);
}

CSocketOperation::~CSocketOperation()
{
	while (!m_sendQueue.empty())
	{
		popSendRequest();
	}
	while (!m_recvQueue.empty())
	{
		popRecvRequest();
	}
	changeEpollOpt(OPERATION_DEL);
}

IO_DATA* CSocketOperation::frontSendRequest()
{
	std::lock_guard<std::mutex> guard(m_sendMutex);
	if (m_sendQueue.empty())
	{
		return nullptr;
	}
	return m_sendQueue.front();
}
void CSocketOperation::popSendRequest()
{
	std::lock_guard<std::mutex> guard(m_sendMutex);
	IO_DATA* pSendData = m_sendQueue.front();
	if (pSendData->nTotalTransferBytes > 0)
	{
		::free(pSendData->pBuf);
		m_mallocSize -= pSendData->nTotalTransferBytes;
	}
	delete pSendData;
	m_sendQueue.pop_front();
	if (m_sendQueue.empty())
	{
		changeEpollOpt(OPERATION_DELWRITE);
	}
}
IO_DATA* CSocketOperation::frontRecvRequest()
{
	std::lock_guard<std::mutex> guard(m_recvMutex);
	if (m_recvQueue.empty())
	{
		return nullptr;
	}
	return m_recvQueue.front();
}
void CSocketOperation::popRecvRequest()
{
	std::lock_guard<std::mutex> guard(m_recvMutex);
	IO_DATA* pRecvData = m_recvQueue.front();
	delete pRecvData;
	m_recvQueue.pop_front();
	if (m_recvQueue.empty())
	{
		changeEpollOpt(OPERATION_DELREAD);
	}

}

int CSocketOperation::pushTcpSendRequest(void* pBuf, int nBytesToSend, IOCALLBACK pCallBack)
{
	if (m_mallocSize > MAX_SEND_BUFFER)
	{
		printf("over max buffer limit, socketfd:%d\n", m_socketFd);
		return -1;
	}
	int remainBytes = nBytesToSend;
	char* remainBuffer = (char*)pBuf;
	while (remainBytes > 0)
	{
		IO_DATA* pSendData = new IO_DATA;
		int tmpBytes = remainBytes > MAX_SEND_LENGTH ? MAX_SEND_LENGTH : remainBytes;
		char* pSendBuffer = (char*)malloc(tmpBytes);
		memcpy(pSendBuffer, remainBuffer, tmpBytes);
		pSendData->pBuf = pSendBuffer;
		pSendData->nTotalTransferBytes = tmpBytes;
		pSendData->nCompletedBytes = 0;
		pSendData->clientAddr = nullptr;
		pSendData->pFunc = pCallBack;
		std::lock_guard<std::mutex> guard(m_sendMutex);
		if (m_sendQueue.empty())
		{
			changeEpollOpt(OPERATION_ADDWRITE);
		}
		m_mallocSize += pSendData->nTotalTransferBytes;
		m_sendQueue.push_back(pSendData);
		remainBytes -= tmpBytes;
		remainBuffer += tmpBytes;
	}
	return 0;
}
int CSocketOperation::pushTcpRecvRequest(void* pBuf, int nBytesToRecv, IOCALLBACK pCallBack)
{
	if (m_recvQueue.size() >= MAX_RECV_QUEUE)
	{
		return -1;
	}
	IO_DATA* pRecvData = new IO_DATA;
	pRecvData->pBuf = (char*)pBuf;
	pRecvData->nTotalTransferBytes = nBytesToRecv;
	pRecvData->nCompletedBytes = 0;
	pRecvData->clientAddr = nullptr;
	pRecvData->pFunc = pCallBack;
	std::lock_guard<std::mutex> guard(m_recvMutex);
	if (m_recvQueue.empty())
	{
		changeEpollOpt(OPERATION_ADDREAD);
	}
	m_recvQueue.push_back(pRecvData);
	return 0;
}

int CSocketOperation::pushTcpAcceptRequest(int fd, AIO_SOCKADDR_IN* clientAddr, IOCALLBACK pCallBack)
{
	if (m_recvQueue.size() >= MAX_RECV_QUEUE)
	{
		return -1;
	}
	IO_DATA* pRecvData = new IO_DATA;
	pRecvData->pBuf = nullptr;
	pRecvData->nTotalTransferBytes = 0;
	pRecvData->nCompletedBytes = 0;
	pRecvData->clientAddr = clientAddr;
	pRecvData->pFunc = pCallBack;
	std::lock_guard<std::mutex> guard(m_recvMutex);
	if (m_recvQueue.empty())
	{
		changeEpollOpt(OPERATION_ADDREAD);
	}
	m_recvQueue.push_back(pRecvData);
	return 0;
}

int CSocketOperation::changeEpollOpt(OPERATION opr)
{
	struct epoll_event ev;
	ev.data.fd = m_socketFd;
	int iRet = -1;
	switch (opr)
	{
	case OPERATION_ADD:
		ev.events = (EPOLLERR | EPOLLHUP);
		iRet = epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_socketFd, &ev);
		break;
	case OPERATION_DEL:
		ev.events = 0;
		iRet = epoll_ctl(m_epollFd, EPOLL_CTL_DEL, m_socketFd, &ev);
		break;
	case OPERATION_ADDREAD:
		ev.events = m_events | EPOLLIN;
		if (m_events == 0)
		{
			iRet = epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_socketFd, &ev);
		}
		else
		{
			iRet = epoll_ctl(m_epollFd, EPOLL_CTL_MOD, m_socketFd, &ev);
		}
		break;
	case OPERATION_DELREAD:
		ev.events = m_events&(~EPOLLIN);
		iRet = epoll_ctl(m_epollFd, EPOLL_CTL_MOD, m_socketFd, &ev);
		break;
	case OPERATION_ADDWRITE:
		ev.events = m_events | EPOLLOUT;
		if (m_events == 0)
		{
			iRet = epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_socketFd, &ev);
		}
		else
		{
			iRet = epoll_ctl(m_epollFd, EPOLL_CTL_MOD, m_socketFd, &ev);
		}
		break;
	case OPERATION_DELWRITE:
		ev.events = m_events&(~EPOLLOUT);
		iRet = epoll_ctl(m_epollFd, EPOLL_CTL_MOD, m_socketFd, &ev);
		break;
	}
	if (iRet == 0)
	{
		m_events = ev.events;
	}
	else
	{
		printf("epoll_ctl failed, opr%d\n", opr);
	}
	return iRet;
}

/************************************************************************************/

CAsyncIO::~CAsyncIO()
{
	destroy();
}

int CAsyncIO::create(int iThreadNum)
{
	memset(m_socketOpertion, 0, sizeof(void*)*ASYNC_MAX_FD);
	m_threadNum = iThreadNum;
	m_epollThread = new EPOLL_THREAD[m_threadNum];
	for (int i = 0; i < m_threadNum; ++i)
	{
		m_epollThread[i].bQuit = false;
		m_epollThread[i].epollFd = epoll_create(ASYNC_MAX_FD);
		if (m_epollThread[i].epollFd == -1)
		{
			printf("epoll_create failed, errorno:%d, errorstr:%s\n",errno, strerror(errno));
			delete[] m_epollThread;
			return -1;
		}
		m_epollThread[i].eventsNum = ASYNC_MAX_FD / m_threadNum;
		m_epollThread[i].events = (epoll_event*)malloc(sizeof(epoll_event) * m_epollThread[i].eventsNum);
		m_epollThread[i].thread = std::thread(&CAsyncIO::epollThreadProc, this, &m_epollThread[i]);
	}
	return 0;
}
int CAsyncIO::destroy()
{
	if (m_epollThread == nullptr)
	{
		return -1;
	}
	for (int i = 0; i < m_threadNum; ++i)
	{
		m_epollThread->bQuit = true;
	}
	for (int i = 0; i < m_threadNum; ++i)
	{
		m_epollThread[i].thread.join();
		close(m_epollThread[i].epollFd);
		free(m_epollThread[i].events);
	}
	delete[] m_epollThread;
	m_epollThread = nullptr;
	return 0;
}
int CAsyncIO::bindSocket(int fd)
{
	int idx = fd%m_threadNum;
	int epollFd = m_epollThread[idx].epollFd;
	if (fd == -1 || epollFd == -1 || fd >= ASYNC_MAX_FD)
	{
		return -1;
	}
	if (m_socketOpertion[fd])
	{
		return -1;
	}
	setBlock(fd, false);
	CSocketOperation* pSocketOpr = new(std::nothrow) CSocketOperation(fd, epollFd);
	if (pSocketOpr == nullptr)
	{
		return -1;
	}
	std::lock_guard<std::recursive_mutex> guard(m_epollThread[idx].epollMutex);
	m_socketOpertion[fd] = pSocketOpr;
	return 0;
}
int CAsyncIO::unbindSocket(int fd)
{
	int idx = fd%m_threadNum;
	int epollFd = m_epollThread[idx].epollFd;
	if (fd == -1 || epollFd == -1 || fd >= ASYNC_MAX_FD)
	{
		return -1;
	}
	if (m_socketOpertion[fd] == nullptr || epollFd != m_socketOpertion[fd]->getEpollFd())
	{
		return -1;
	}
	std::lock_guard<std::recursive_mutex> guard(m_epollThread[idx].epollMutex);
	delete m_socketOpertion[fd];
	m_socketOpertion[fd] = nullptr;
	return 0;
}
int CAsyncIO::asyncRecv(int fd, void* pBuf, int nBytesToRecv, IOCALLBACK callBack)
{
	if (fd >= ASYNC_MAX_FD || fd < 0)
	{
		return -1;
	}
	if (nBytesToRecv < 0)
	{
		return -1;
	}
	if (m_socketOpertion[fd] == nullptr)

	{
		return -1;
	}
	return m_socketOpertion[fd]->pushTcpRecvRequest(pBuf, nBytesToRecv, callBack);
}
int CAsyncIO::asyncSend(int fd, void* pBuf, int nBytesToSend, IOCALLBACK callBack)
{
	if (fd >= ASYNC_MAX_FD || fd < 0)
	{
		return -1;
	}
	if (nBytesToSend < 0)
	{
		return -1;
	}
	if (m_socketOpertion[fd] == nullptr)

	{
		return -1;
	}
	return m_socketOpertion[fd]->pushTcpSendRequest(pBuf, nBytesToSend, callBack);
}
int CAsyncIO::asyncAccept(int fd, AIO_SOCKADDR_IN* clientAddr, IOCALLBACK callBack)
{
	if (m_socketOpertion[fd] == nullptr)
	{
		return -1;
	}
	return m_socketOpertion[fd]->pushTcpAcceptRequest(fd, clientAddr, callBack);
}
void CAsyncIO::epollThreadProc(void* ptr)
{
	//设置线程优先级
	struct sched_param param;
	int policy = SCHED_FIFO;
	pthread_t thread_id = pthread_self();
	param.sched_priority = 1;
	pthread_setschedparam(thread_id, policy, &param);
	
	EPOLL_THREAD* pEpollThread = (EPOLL_THREAD*)ptr;
	int iIOFds = -1;
	while (pEpollThread->bQuit == false)
	{
		iIOFds = epoll_wait(pEpollThread->epollFd, pEpollThread->events, pEpollThread->eventsNum, 500);
		if (iIOFds <= 0)
		{
			continue;
		}
		std::lock_guard<std::recursive_mutex> guard(pEpollThread->epollMutex);
		for (int i = 0; i < iIOFds; ++i)
		{
			int socketFd = (int)pEpollThread->events[i].data.fd;
			if (pEpollThread->events[i].events & EPOLLIN)
			{
				IO_DATA* pRecvData = nullptr;
				if (m_socketOpertion[socketFd])
				{
					pRecvData = m_socketOpertion[socketFd]->frontRecvRequest();
				}
				if (pRecvData == nullptr)
				{
					continue;
				}
				int iRet = 0;
				if (pRecvData->pBuf == nullptr && pRecvData->clientAddr != nullptr)
				{
					socklen_t cliaddr_len = sizeof(sockaddr_in);
					iRet = ::accept(socketFd, (sockaddr*)&pRecvData->clientAddr->SA, &cliaddr_len);
					pRecvData->nCompletedBytes = iRet;
				}
				else
				{
					iRet = ::recv(socketFd, pRecvData->pBuf, pRecvData->nTotalTransferBytes, 0);
					pRecvData->nCompletedBytes = (iRet > 0) ? iRet : 0;
				}
				if (iRet <= 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
				{
					continue;
				}
				pRecvData->nErrorCode = (iRet > 0) ? 0 : errno;
				if (pRecvData->pFunc)
				{
					pRecvData->pFunc(pRecvData->nErrorCode, pRecvData->nCompletedBytes);
				}
				if (m_socketOpertion[socketFd])
				{
					m_socketOpertion[socketFd]->popRecvRequest();
				}
			}
			if (pEpollThread->events[i].events & EPOLLOUT)
			{
				IO_DATA* pSendData = nullptr;
				if (m_socketOpertion[socketFd])
				{
					pSendData = m_socketOpertion[socketFd]->frontSendRequest();
				}
				if (pSendData == nullptr)
				{
					continue;
				}
				int readyToSendBytes = pSendData->nTotalTransferBytes - pSendData->nCompletedBytes;
				int iRet = ::send(socketFd, pSendData->pBuf + pSendData->nCompletedBytes, readyToSendBytes,0);
				if (iRet >= 0 && iRet != readyToSendBytes)
				{
					pSendData->nCompletedBytes += iRet;
					continue;
				}
				else if (iRet > 0 && iRet == readyToSendBytes)
				{
					pSendData->nCompletedBytes = pSendData->nTotalTransferBytes;
					pSendData->nErrorCode = 0;
					if (pSendData->pFunc)
					{
						pSendData->pFunc(pSendData->nErrorCode, pSendData->nCompletedBytes);
					}
					if (m_socketOpertion[socketFd])
					{
						m_socketOpertion[socketFd]->popSendRequest();
					}
				}
				else if (iRet < 0)
				{
					pSendData->nErrorCode = errno;
					if (pSendData->pFunc)
					{
						pSendData->pFunc(errno, pSendData->nCompletedBytes);
					}
				}
			}
			if ((pEpollThread->events[i].events&EPOLLERR) || (pEpollThread->events[i].events & EPOLLHUP))
			{

			}
		}
	}
}