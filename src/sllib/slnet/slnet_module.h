#ifndef SL_SLNET_MODULE_H
#define SL_SLNET_MODULE_H
#include "slnet.h"
#include "slsingleton.h"
#include "sllistener.h"
#include "slnetwork_interface.h"
#include "slevent_dispatcher.h"

namespace sl
{
namespace network
{
class CSLNetModule: public ISLNet, public CSingleton<CSLNetModule>
{
public:
	CSLNetModule();
	~CSLNetModule();
public:
	virtual ISLListener* SLAPI createListener();
	virtual ISLConnector* SLAPI createConnector();

	inline NetworkInterface* getNetworkInterface() {return m_networkInterface;}
private:
	EventDispatcher					m_dispatcher;
	NetworkInterface*				m_networkInterface;
};
}
}

#endif