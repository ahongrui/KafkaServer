#include "ApiVersionHandle.h"
#include "utils.h"


unsigned short ArrayApiVersion[44][3] = 
{
	{ htons(0), htons(0), htons(7) }, //Produce
	{ htons(1), htons(0), htons(10)}, //Fetch
	{ htons(2), htons(0), htons(5) }, //OffSets
	{ htons(3), htons(0), htons(4) }, //Metadata
	{ htons(4), htons(0), htons(2) }, //LeaderAndIsr
	{ htons(5), htons(0), htons(1) }, //StopReplica
	{ htons(6), htons(0), htons(5) }, //UpdateMetadata
	{ htons(7), htons(0), htons(2) }, //ControlledShutDown
	{ htons(8), htons(0), htons(6) }, //OffsetCommit
	{ htons(9), htons(0), htons(5) }, //OffsetFetch
	{ htons(10), htons(0), htons(2) }, //FindCoodinator
	{ htons(11), htons(0), htons(4) }, //JoinGroup
	{ htons(12), htons(0), htons(2) }, //Heartbeat
	{ htons(13), htons(0), htons(2) }, //LeaveGroup
	{ htons(14), htons(0), htons(2) }, //SyncGroup
	{ htons(15), htons(0), htons(2) }, //DescribeGroups
	{ htons(16), htons(0), htons(2) }, //ListGroups
	{ htons(17), htons(0), htons(1) }, //SaslHandshake
	{ htons(18), htons(0), htons(2) }, //ApiVersions
	{ htons(19), htons(0), htons(3) }, //CreateTopics
	{ htons(20), htons(0), htons(3) }, //DeleteTopics
	{ htons(21), htons(0), htons(1) }, //DeleteRecords
	{ htons(22), htons(0), htons(1) }, //InitProducerId
	{ htons(23), htons(0), htons(2) }, //OffsetForLeaderEn
	{ htons(24), htons(0), htons(1) }, //AddPartitionToTx
	{ htons(25), htons(0), htons(1) }, //AddOffsetsToTxn
	{ htons(26), htons(0), htons(1) }, //EndTxn
	{ htons(27), htons(0), htons(0) }, //WriteTxnMarkers
	{ htons(28), htons(0), htons(2) }, //TxnOffsetfCommit
	{ htons(29), htons(0), htons(1) }, //DescribeAcls
	{ htons(30), htons(0), htons(1) }, //CreateAcls
	{ htons(31), htons(0), htons(1) }, //DeleteAcls
	{ htons(32), htons(0), htons(2) }, //DescribeConfigs
	{ htons(33), htons(0), htons(1) }, //AlterConfigs
	{ htons(34), htons(0), htons(1) }, //AlterReplicaLogDirs
	{ htons(35), htons(0), htons(1) }, //DescribeLogDirs
	{ htons(36), htons(0), htons(0) }, //SaslAuthenticate 
	{ htons(37), htons(0), htons(1) }, //CreatePartitions 
	{ htons(38), htons(0), htons(1) }, //CrteatDelegationToken
	{ htons(39), htons(0), htons(1) }, //RenewDelegationToken
	{ htons(40), htons(0), htons(1) }, //ExpireDelegationToken
	{ htons(41), htons(0), htons(1) }, //DescribeDelegationToken
	{ htons(42), htons(0), htons(1) }, //DeteleGroups
	{ htons(43), htons(0), htons(0) }  //ElectLeaders
};
CApiVersionHandle::CApiVersionHandle()
{
	m_minVersion = 0;
	m_maxVersion = 2;
	m_apiKey = 18;
}
int CApiVersionHandle::minVersion() const
{
	return m_minVersion;
}
int CApiVersionHandle::maxVersion() const
{
	return m_maxVersion;
}
int CApiVersionHandle::key() const
{
	return m_apiKey;
}
int CApiVersionHandle::handle(RequestHeader* pHeader, char* readBuf, char* writeBuf, void* pUsrData)
{
	/*
	ApiVersion Response: length(int32) + correlation ID(int32) + error(int16) + Api Version Count(int32) + [Api Version{}]
	Api Version : API KEY(int16) + Min Version(int16) + Max Version(int16)
	*/
	char* ptr = writeBuf + 4;
	writeInt(pHeader->correlation, ptr); //correlation ID
	writeShort(ErrorCode_None, ptr);
	writeInt(44, ptr); // Api Version Count
	memcpy(ptr, ArrayApiVersion, sizeof(ArrayApiVersion));
	ptr += sizeof(ArrayApiVersion);

	int length = ptr - writeBuf;
	*(int*)writeBuf = htonl(length - 4);
	return length;
}
