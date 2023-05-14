#include "SaslAuthenticateHandle.h"
#include "Session.h"
#include "utils.h"
#include <vector>
#include <map>
#include "gssapi/gssapi.h"
#include "gssapi/gssapi_ext.h"
#include "gssapi/gssapi_generic.h"
#include "gssapi/gssapi_krb5.h"
CSaslAuthenticateHandle::CSaslAuthenticateHandle()
{
	m_minVersion = 0;
	m_maxVersion = 1;
	m_apiKey = 36;
	init();
}
int CSaslAuthenticateHandle::minVersion() const
{
	return m_minVersion;
}
int CSaslAuthenticateHandle::maxVersion() const
{
	return m_maxVersion;
}
int CSaslAuthenticateHandle::key() const
{
	return m_apiKey;
}
int CSaslAuthenticateHandle::handle(RequestHeader* pHeader, char* readBuf, char* writeBuf, void* pUsrData)
{
	CSession* pSession = (CSession*)pUsrData;
	std::string mechanism = pSession->GetMechanism();
	char* ptr = readBuf;
	if (mechanism == "GSSAPI")
	{
		if (pSession->GetAuthStatus() == 0)
		{
			ptr += 2; //SASL Authentication Bytes
			int length = readShort(ptr); //Authentication Bytes Length
			OM_uint32 minor_status = 0;
			gss_buffer_desc recvToken;
			recvToken.length = length;
			recvToken.value = ptr;
			gss_name_t clientName;
			gss_OID oid;
			gss_buffer_desc sendToken;
			OM_uint32 ret_flags;
			gss_ctx_id_t context = nullptr;
			//获取安全上下文
			OM_uint32 major_status = gss_accept_sec_context(&minor_status, &context, m_serverCreds,
				&recvToken, GSS_C_NO_CHANNEL_BINDINGS, &clientName, &oid, &sendToken, &ret_flags, nullptr, nullptr);
			if (sendToken.length > 0 && sendToken.value != nullptr)
			{
				gss_release_buffer(&minor_status, &sendToken);
			}
			if (major_status != GSS_S_COMPLETE)
			{
				if (context != GSS_C_NO_CONTEXT)
				{
					gss_delete_sec_context(&minor_status, &context, GSS_C_NO_BUFFER);
				}
				printf("major_status:%d, minor_status:%d, failed to accept security context\n", major_status, minor_status);
				return 0;
			}
			ptr = writeBuf + 4;
			writeInt(pHeader->correlation, ptr);
			writeShort(ErrorCode_None, ptr); //ERROR
			writeShort(0xffff, ptr); //error message
			writeShort(0, ptr); //SASL Authentication

			char buf[4] = {0x1, 0x2, 0x3, 0x4};
			gss_buffer_desc input;
			gss_buffer_desc output;
			input.length = 4;
			input.value = buf;
			major_status = gss_wrap(&minor_status, context, 1, GSS_C_QOP_DEFAULT, &input, nullptr, &output);
			if (major_status != GSS_S_COMPLETE)
			{
				printf("major_status:%d, minor_status:%d, failed to wrap ssf\n", major_status, minor_status);
				return 0;
			}
			writeShort(output.length, ptr);
			memcpy(ptr, output.value, output.length);
			ptr += output.length;

			OM_uint32 minor{};
			gss_release_buffer(&minor, &output);
			length = ptr - writeBuf;
			*(int*)writeBuf = htonl(length - 4);
			pSession->SetAuthStatus(1);
			return length;
		}
		else if (pSession->GetAuthStatus() == 1)
		{
			ptr = writeBuf + 4;
			writeInt(pHeader->correlation, ptr);
			writeShort(ErrorCode_None, ptr);
			writeShort(0xffff, ptr);
			writeShort(0, ptr);
			writeShort(0, ptr);
			int length = ptr - writeBuf;
			*(int*)writeBuf = htonl(length - 4);
			return length;
		}
	}
	else if (mechanism == "PLAIN")
	{
		ptr = writeBuf + 4;
		writeInt(pHeader->correlation, ptr);
		writeShort(ErrorCode_None, ptr);
		writeShort(0xffff, ptr);
		writeShort(0, ptr);
		writeShort(0, ptr);
		int length = ptr - writeBuf;
		*(int*)writeBuf = htonl(length - 4);
		return length;
	}
	return 0;
}

void CSaslAuthenticateHandle::init()
{
	m_servicePrimary = "kafka-server";
	m_keytab = "./kafka-server.keytab";
	krb5_gss_register_acceptor_identity(m_keytab.c_str());
	OM_uint32 minor_status;
	gss_name_t server_name;
	gss_buffer_desc service_name;
	service_name.length = m_servicePrimary.size() + 1;
	service_name.value = const_cast<char*>(m_servicePrimary.c_str());
	//导入客户端名称
	OM_uint32 major_status = gss_import_name(&minor_status, &service_name, GSS_C_NT_HOSTBASED_SERVICE, &server_name);
	if (major_status != GSS_S_COMPLETE)
	{
		printf("major_status:%d, minor_status:%d, failed to import service principal\n", major_status, minor_status);
		return;
	}
	//获取凭证
	major_status = gss_acquire_cred(&minor_status, server_name, 0, GSS_C_NO_OID_SET, GSS_C_ACCEPT, &m_serverCreds, NULL, NULL);
	if (major_status != GSS_S_COMPLETE)
	{
		printf("major_status:%d, minor_status:%d, failed to acquire credentials\n", major_status, minor_status);
		return;
	}
	gss_release_name(&minor_status, &server_name);
}