#include "HawkZmqManager.h"
#include "HawkOSOperator.h"
#include "zmq.h"

namespace Hawk
{
	HAKW_SINGLETON_IMPL(ZmqManager);

	HawkZmqManager::HawkZmqManager()
	{
		m_pZmqCtx = 0;
	}

	HawkZmqManager::~HawkZmqManager()
	{
		ZmqMap::iterator it = m_mZmq.begin();
		for (;it != m_mZmq.end();++it)
		{
			HawkZmq* pZmq = it->second;

			if (pZmq)
				pZmq->Close();
			
			HAWK_RELEASE(pZmq);
		}
		m_mZmq.clear();

		if(m_pZmqCtx)
		{
			zmq_ctx_destroy(m_pZmqCtx);
			m_pZmqCtx = 0;
		}
	}

	Bool  HawkZmqManager::SetupZmqCtx(Int32 iThreads)
	{
		if (!m_pZmqCtx)
		{
			m_pZmqCtx = zmq_ctx_new();
			if (m_pZmqCtx)
			{
				zmq_ctx_set(m_pZmqCtx, ZMQ_IO_THREADS, iThreads);
			}
		}
		return m_pZmqCtx != 0;
	}

	HawkZmq* HawkZmqManager::CreateZmq(Int32 iType)
	{
		if (m_pZmqCtx || SetupZmqCtx())
		{
			void* pHdl = zmq_socket(m_pZmqCtx, iType);
			if (pHdl)
			{
				Int32 iLinger = 0;
				zmq_setsockopt (pHdl, ZMQ_LINGER, &iLinger, sizeof(iLinger));

				HawkZmq* pZmq = new HawkZmq();
				if (pZmq->Init(iType, pHdl))
				{
					m_mZmq[pZmq] = pZmq;
					return pZmq;
				}
				else
				{
					zmq_close(pHdl);
					HAWK_RELEASE(pZmq);					
				}				
			}
		}
		return 0;
	}

	Bool HawkZmqManager::CloseZmq(HawkZmq*& pZmq)
	{
		if (pZmq)
		{
			ZmqMap::iterator it = m_mZmq.find(pZmq);
			if (it != m_mZmq.end())
			{
				if (pZmq)
					pZmq->Close();

				m_mZmq.erase(it);
				HAWK_RELEASE(pZmq);
				return true;
			}
		}
		return false;
	}

	Bool HawkZmqManager::ProxyZmq(HawkZmq* pFrontend, HawkZmq* pBackend, Bool bBothway, Int32 iTimeout, Bool bOnce)
	{
		HawkAssert(pFrontend && pBackend);
		if (!pFrontend || !pBackend)
			return false;

		zmq_msg_t sMsg;
		if (zmq_msg_init(&sMsg) != HAWK_OK)
			return false;

		zmq_pollitem_t items[] = 
		{
			{ pFrontend->GetHandle(), 0, ZMQ_POLLIN, 0 },
			{ pBackend->GetHandle(),  0, ZMQ_POLLIN, 0 }
		};

		do
		{
			if (zmq_poll(items, 2, iTimeout) < 0)
				return false;

			if (items[0].revents & ZMQ_POLLIN) 
			{
				while (true) 
				{
					if (zmq_recvmsg(items[0].socket, &sMsg, 0) < 0)
						return false;

					Int32  iRecvMore = 0;
					Size_t iLen = sizeof(iRecvMore);
					if (zmq_getsockopt(items[0].socket, ZMQ_RCVMORE, &iRecvMore, &iLen) < 0)
						return false;

					if (zmq_sendmsg(items[1].socket, &sMsg, iRecvMore? ZMQ_SNDMORE: 0) < 0)
						return false;

					if (iRecvMore == 0)
						break;
				}
			}
		
			if (bBothway && (items[1].revents & ZMQ_POLLIN)) 
			{
				while (true) 
				{
					if (zmq_recvmsg(items[1].socket, &sMsg, 0) < 0)
						return false;

					Int32  iRecvMore = 0;
					Size_t iLen = sizeof(iRecvMore);
					if (zmq_getsockopt(items[1].socket, ZMQ_RCVMORE, &iRecvMore, &iLen) < 0)
						return false;

					if (zmq_sendmsg(items[0].socket, &sMsg, iRecvMore? ZMQ_SNDMORE: 0) < 0)
						return false;

					if (iRecvMore == 0)
						break;
				}
			}
		}while(!bOnce);

		zmq_msg_close(&sMsg);
		return true;
	}
}
