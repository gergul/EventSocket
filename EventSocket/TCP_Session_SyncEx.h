#pragma once
#include "TCP_Server.h"

class TCP_Session_SyncEx :
	public TCP_Session
{
public:
	TCP_Session_SyncEx();
	virtual ~TCP_Session_SyncEx();

	virtual int send(const char* data, int datalen = 0) override;
	
protected:
	///������ݵ��¼�
	//  ��õ��Ǵ���ʶ��ԭʼ����
	virtual void onRecv(const char* data, int dataLen) override;
	///������ݵ��¼�
	//  ��õ��ǲ�����ʶ��ԭʼ����
	virtual void onRecvEx(const char* data, int dataLen);

protected:
	std::string m_curSendWaitId;
};

