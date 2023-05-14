#ifndef  __IHANDLE_H__
#define  __IHANDLE_H__
#include <string>
typedef struct
{
	short key;
	short version;
	int	correlation;
	short client_id_size;
	//std::string client_id;
	//std::string tagl;
}RequestHeader;

enum KafkaErrorCode
{
	ErrorCode_None = 0,
	ErrorCode_UnsupportedSaslMechanism = 33,
	ErrorCode_UnsupportedVersion = 35
};

class IProtocolHandle
{
public:
	virtual int minVersion() const = 0;
	virtual int maxVersion() const = 0;
	virtual int key() const = 0;
	virtual int handle(RequestHeader* pHeader, char* readBuf, char* writeBuf, void* pUsrData) = 0;
};
#endif // __IHANDLE_H__
