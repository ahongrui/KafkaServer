#ifndef __ASYNCIO_H__
#define __ASYNCIO_H__
#include <functional>
#include <mutex>
#include <deque>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <sys/epoll.h>
#include <netinet/in.h>
#define ASYNC_MAX_FD 65536
typedef std::function<void(int nErrorCode, int nNumberOfbytes)>   IOCALLBACK;
typedef struct 
{
	bool			bQuit;		//�߳��˳���־
	int				epollFd;	//epoll���
	int				eventsNum;	//epoll�����ע��fd��Ŀ
	epoll_event*	events; //�¼����
	std::recursive_mutex epollMutex; //ȷ��unbind��epoll����������
	std::thread		thread;
}EPOLL_THREAD;

typedef struct  
{
	union
	{
		struct sockaddr_in	sin4;	//ipv4
		struct sockaddr_in6 sin6;	//ipv6
	}SA;
}AIO_SOCKADDR_IN;

typedef enum 
{
	OPERATION_ADD,
	OPERATION_DEL,
	OPERATION_ADDREAD,
	OPERATION_DELREAD,
	OPERATION_ADDWRITE,
	OPERATION_DELWRITE
}OPERATION;
typedef struct 
{
	int		iSocket;
	char*	pBuf;					//���ݻ���
	int		nTotalTransferBytes;	//������Ҫ��ɵĳ���
	int		nCompletedBytes;		//�Ѿ�������ɵĳ���
	int		nErrorCode;				//������
	IOCALLBACK pFunc;				//�ص�����
	AIO_SOCKADDR_IN* clientAddr;	//�Զ˵�ַ
}IO_DATA;

class CSocketOperation
{
public:
	CSocketOperation(int socketFd, int epollFd);
	~CSocketOperation();
	int pushTcpSendRequest(void* pBuf, int nBytesToSend, IOCALLBACK pCallBack);
	int pushTcpRecvRequest(void* pBuf, int nBytesToRecv, IOCALLBACK pCallBack);
	int pushTcpAcceptRequest(int fd, AIO_SOCKADDR_IN* clientAddr, IOCALLBACK pCallBack);

	int getEpollFd() { return m_epollFd; }
	IO_DATA* frontSendRequest();
	void popSendRequest();
	IO_DATA* frontRecvRequest();
	void popRecvRequest();
private:
	int changeEpollOpt(OPERATION opr);
public:
	std::mutex m_sendMutex;
	std::mutex m_recvMutex;
private:
	unsigned int m_events;
	int m_socketFd;
	int m_epollFd;
	int m_memLimitSize;
	std::atomic<int> m_mallocSize;
	std::deque<IO_DATA*> m_sendQueue;
	std::deque<IO_DATA*> m_recvQueue;
};

class CAsyncIO
{
public:
	virtual ~CAsyncIO();
	int create(int iThreadNum);
	int destroy();
	int bindSocket(int fd);
	int unbindSocket(int fd);
	int asyncRecv(int fd, void* pBuf, int nBytesToRecv, IOCALLBACK callBack = nullptr);
	int asyncSend(int fd, void* pBuf, int nBytesToSend, IOCALLBACK callBack = nullptr);
	int asyncAccept(int fd, AIO_SOCKADDR_IN* clientAddr, IOCALLBACK callBack = nullptr);
private:
	void epollThreadProc(void* ptr);
private:
	int m_threadNum; //epoll�߳���
	EPOLL_THREAD* m_epollThread;
	CSocketOperation* m_socketOpertion[ASYNC_MAX_FD];
};

#endif
