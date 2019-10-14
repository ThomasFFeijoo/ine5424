// EPOS IP Protocol Implementation

#include <utility/string.h>
#include <network/my_protocol.h>
#include <system.h>

#ifdef __mp__

__BEGIN_SYS
// Class attributes
My_Protocol * My_Protocol::_networks[];
My_Protocol::Observed My_Protocol::_observed;

// Methods
My_Protocol::~My_Protocol()
{
    _nic->detach(this, NIC<Ethernet>::PROTO_MP);
}

int My_Protocol::send(const void * data, unsigned int size)
{
    db<My_Protocol>(TRC) << "My_Protocol::send()" << endl;
    //TODO
}

int My_Protocol::receive(void * d, unsigned int s)
{
    //TODO
}

void My_Protocol::update(NIC<Ethernet>::Observed * obs, const NIC<Ethernet>::Protocol & prot, Buffer * buf)
{
   //TODO
}

__END_SYS

#endif