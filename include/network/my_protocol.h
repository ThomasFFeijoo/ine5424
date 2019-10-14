// EPOS IP Protocol Declarations

#ifndef __simple_protocol_h
#define __simple_protocol_h

#include <system/config.h>

#ifdef __mp__

#include <utility/bitmap.h>
#include <machine/nic.h>

__BEGIN_SYS

class My_Protocol: private NIC<Ethernet>::Observer
{

    friend class System;
    friend class Network_Common;

public:
    //static const unsigned int TIMEOUT = Traits<IP>::TIMEOUT * 1000000;

    // IP Protocol Id
    //static const unsigned int PROTOCOL = Ethernet::PROTO_IP;
    // Addresses
    typedef Ethernet::Address MAC_Address;
    typedef NIC_Common::Address<4> Address;
    
    //typedef unsigned char Protocol and enum

    // Buffers received by the NIC, eventually linked into a list
    typedef Ethernet::Buffer Buffer;

    // IP and NIC observer/d
    typedef Data_Observer<Buffer, Protocol> Observer;
    typedef Data_Observed<Buffer, Protocol> Observed;

    //class Header removed
    //class Pseudo_Header removed
    //class Packet removed
    //class Fragment removed
    //class Route removed
    

protected:
    template<unsigned int UNIT = 0>
    My_Protocol(unsigned int nic = UNIT);
public:
    ~My_Protocol();
    // void reconfigure

    //Leaving only nic and address because that's what I need right now
    NIC<Ethernet> * nic() { return _nic; }
    const Address & address() const { return _address; }

    static My_Protocol * get_by_nic(unsigned int unit) {
        if(unit >= Traits<Ethernet>::UNITS) {
            db<My_Protocol>(WRN) << "My_Protocol::get_by_nic: requested unit (" << unit << ") does not exist!" << endl;
            return 0;
        } else
            return _networks[unit];
    }
    //static Buffer * alloc(const Address & to, const Ethernet::Protocol & prot, unsigned int once, unsigned int payload);
    static int send(const void * data, unsigned int size);
    static int receive(void * data, unsigned int size);
    static const unsigned int mtu() { return MTU; }

    static void attach(Data_Observer<Buffer> * obs) { _observed.attach(obs); }
    static void detach(Data_Observer<Buffer> * obs) { _observed.detach(obs); }

    friend Debug & operator<<(Debug & db, const My_Protocol & mp) {
        db << "{a=" << mp._address
           << ",nic=" << &mp._nic
           << "}";
        return db;
    }

private:
    void config_by_mac() { _address[sizeof(Address) -1] = _nic->address()[sizeof(MAC_Address) - 1]; }

    void update(Ethernet::Observed * obs, const Ethernet::Protocol & prot, Buffer * buf);

    static bool notify(Buffer * buf) { return _observed.notify(buf); }

    static void init(unsigned int unit);

protected:
    NIC<Ethernet> * _nic;
    Address _address;
    static My_Protocol * _networks[Traits<Ethernet>::UNITS];
    static Observed _observed; 
};

__END_SYS

#endif

#endif