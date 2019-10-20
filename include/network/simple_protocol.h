// EPOS IP Protocol Declarations

#ifndef __simple_protocol_h
#define __simple_protocol_h

#include <system/config.h>
#include <utility/bitmap.h>
#include <machine/nic.h>

#ifdef __sp__

// TODO: what is this?
__BEGIN_SYS

class Simple_Protocol: private NIC<Ethernet>::Observer {

    friend class System;
    friend class Network_Common;

public:
    // Buffers received by the NIC, eventually linked into a list
    typedef Ethernet::Buffer Buffer;
    // IP and NIC observer/d
    typedef Data_Observer<Buffer, Protocol> Observer;
    typedef Data_Observed<Buffer, Protocol> Observed;

protected:

    template<unsigned int UNIT = 0>
    Simple_Protocol(unsigned int nic = UNIT);

public:

    static int send(const void * data, unsigned int size);

    static int receive(void * data, unsigned int size);

    static const unsigned int mtu() { return MTU; }

private:

    void update(Ethernet::Observed * obs, const Ethernet::Protocol & prot, Buffer * buf);

    static bool notify(Buffer * buf) { return _observed.notify(buf); }

protected:

    NIC<Ethernet> * _nic;
    Address _address;
    static Observed _observed;

};

__END_SYS

#endif
