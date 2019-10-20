// EPOS IP Protocol Declarations

#ifndef __simple_protocol_h
#define __simple_protocol_h

#include <system/config.h>
#include <machine/nic.h>

#ifdef __sp__

// TODO: what is this?
__BEGIN_SYS

class Simple_Protocol:
    private NIC<Ethernet>::Observer,
    private Concurrent_Observer<Ethernet::Buffer, Ethernet::Protocol> {

public:

    typedef Ethernet::Buffer Buffer;

public:

    Simple_Protocol(unsigned int nic = 0) :
            _nic(Traits<Ethernet>::DEVICES::Get<0>::Result::get(nic))
    {
        _nic->attach(this, Ethernet::PROTO_SP);
    }

    const Address & address() {
        return _nic->address();
    }

    int send(const Address & dst, const void * data, unsigned int size) {
         return _nic->send(dst, Ethernet::PROTO_SP, data, size);
    }

    int receive(void * data, unsigned int size) {

    }

    void update(Observed *obs, const Ethernet::Protocol & prot, Buffer * buf) {

    }

protected:

    NIC<Ethernet> * _nic;

};

__END_SYS

#endif
#endif
