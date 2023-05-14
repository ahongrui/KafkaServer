#include "MetadataHandle.h"
#include "utils.h"
#include <vector>
#include <string.h>

#define IP			"127.0.0.1"
#define NODE_ID		1001
#define CLUSTER_ID "mG7fG31ZSLOA6bVpXgVbnA"

CMetadataHandle::CMetadataHandle()
{
	m_minVersion = 0;
	m_maxVersion = 7;
	m_apiKey = 3;
}
int CMetadataHandle::minVersion() const
{
	return m_minVersion;
}
int CMetadataHandle::maxVersion() const
{
	return m_maxVersion;
}
int CMetadataHandle::key() const
{
	return m_apiKey;
}
int CMetadataHandle::handle(RequestHeader* pHeader, char* readBuf, char* writeBuf, void* pUsrData)
{
	//先获取topic列表
	std::vector<std::string> vecTopicName;
	char* ptr = readBuf;
	int topicNum = readInt(ptr);
	for (int i = 0; i < topicNum; ++i)
	{
		short len = readShort(ptr);
		vecTopicName.push_back(readString(ptr, len));
	}

	ptr = writeBuf + 4;
	writeInt(pHeader->correlation, ptr); //correlation ID

	writeInt(0, ptr); //Throtlle Time 流控时间
	//broker metadata: broker count(int32) + [broker{}] + Cluster ID len(int16) + Cluster ID + Controller ID(int32)
	//broker{}: node id(int32) + IP length(int16) + IP + port(int32) + rack(int16)
	writeInt(1, ptr); //broker count
	writeInt(NODE_ID, ptr); //node id
	short ipLen = strlen(IP);
	writeShort(ipLen, ptr); //IP length
	writeString(IP, ipLen, ptr); // IP
	writeInt(SERV_PORT, ptr); //port
	writeShort(0xffff, ptr); //rack
	short clusterLen = strlen(CLUSTER_ID);
	writeShort(clusterLen, ptr);//Cluster ID len
	writeString(CLUSTER_ID, clusterLen, ptr); //Cluster ID
	writeInt(NODE_ID, ptr);

	// Topic metadata: Topic Count(int32) + [topic{}]
	// topic{}: error(int16) + topic name len(int16) + topic name + Is Internal(int8) + partition count(int32) +[partition{}]
	// partition{}: error(int16) + partition Id(int32) + Lead Id(int32) + replicas count(int32) + [replicasId{}]
	//		+ Caught_Up replicas count(int32) + Caught_Up replicas ID(int32)
	writeInt(topicNum, ptr); //Topic Count
	for (int j = 0; j < topicNum; ++j)
	{
		writeShort(0, ptr);//error
		int size = vecTopicName[j].size();
		writeShort(size, ptr);//topic name len
		writeString(vecTopicName[j].c_str(), size, ptr);//topic name
		writeChar(0x1, ptr);//Is Internal
		writeInt(1, ptr);//partition count
		writeShort(0,ptr);//error
		writeInt(0, ptr); //partition Id
		writeInt(NODE_ID, ptr); // Lead ID
		writeInt(1, ptr);//replicas count
		writeInt(NODE_ID, ptr);//replicasId
		writeInt(1, ptr);//Caught_Up replicas count
		writeInt(NODE_ID, ptr); //Caught_Up replicas ID
	}
	int length = ptr - writeBuf;
	*(int*)writeBuf = htonl(length - 4);
	return length;
}