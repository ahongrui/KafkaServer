#include <string>
#include <map>
#include <mutex>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <condition_variable>
#include "ApiVersionHandle.h"
#include "InitProducerHandle.h"
#include "MetadataHandle.h"
#include "ProduceHandle.h"
#include "SaslAuthenticateHandle.h"
#include "SaslHandshakeHandle.h"
#include "AsyncIO.h"
#include "Session.h"

#define SERV_PORT 19092

std::map<int, IProtocolHandle*> g_mapProtocolHandle;
void InitHandle()
{
	IProtocolHandle* pHandle = new CApiVersionHandle;
	g_mapProtocolHandle.insert(std::make_pair(pHandle->key(), pHandle));
	
	pHandle = new CMetadataHandle;
	g_mapProtocolHandle.insert(std::make_pair(pHandle->key(), pHandle));

	pHandle = new CProduceHandle;
	g_mapProtocolHandle.insert(std::make_pair(pHandle->key(), pHandle));

	pHandle = new CSaslHandshakeHandle;
	g_mapProtocolHandle.insert(std::make_pair(pHandle->key(), pHandle));

	pHandle = new CSaslAuthenticateHandle;
	g_mapProtocolHandle.insert(std::make_pair(pHandle->key(), pHandle));

	pHandle = new CInitProducerHandle;
	g_mapProtocolHandle.insert(std::make_pair(pHandle->key(), pHandle));
}
int listenfd = 0;
CAsyncIO asyncIO;
AIO_SOCKADDR_IN g_cliaddr;
void acceptCallBack(int nErrorCode, int nNumberOfBytes)
{
	int connfd = nNumberOfBytes;
	printf("acceptCallBack accpet listend:%d connfd:%d\n", listenfd, connfd);
	char str[20] = {0};
	inet_ntop(AF_INET, &g_cliaddr.SA.sin4.sin_addr, str, 16);
	printf("acceptCallBack receive from %s at port %d\n", str, ntohs(g_cliaddr.SA.sin4.sin_port));
	new CSession(&asyncIO, connfd);
	int iRet = asyncIO.asyncAccept(listenfd, &g_cliaddr, acceptCallBack);
	if (iRet != 0) printf("iRet :%d\n",iRet);
}

int main()
{
	std::map<int, IProtocolHandle*> mapProtocoHandler;
	IProtocolHandle* pHandle = new CApiVersionHandle;
	mapProtocoHandler.insert(std::make_pair(pHandle->key(), pHandle));
	pHandle = new CInitProducerHandle;
	mapProtocoHandler.insert(std::make_pair(pHandle->key(), pHandle));
	pHandle = new CInitProducerHandle;
	mapProtocoHandler.insert(std::make_pair(pHandle->key(), pHandle));
	struct sockaddr_in servaddr;
	socklen_t cliaddr_len;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	memset(&servaddr,0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd,1024);
	InitHandle();
	int iRet = asyncIO.create(1);
	if (iRet != 0) return -1;
	iRet = asyncIO.bindSocket(listenfd);
	if (iRet != 0) return -1;
	iRet = asyncIO.asyncAccept(listenfd, &g_cliaddr, acceptCallBack);
	if (iRet != 0) return -1;
	std::mutex mtx;
	std::unique_lock<std::mutex> lck(mtx);
	std::condition_variable cv;
	cv.wait(lck, [&]() {return 0; });
	return 0;
}


