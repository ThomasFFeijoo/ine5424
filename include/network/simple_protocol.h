// EPOS IP Protocol Declarations

#ifndef __simple_protocol_h
#define __simple_protocol_h

#include <system/config.h>
#include <machine/nic.h>
#include <synchronizer.h>

__BEGIN_SYS

class Simple_Protocol:
    private NIC<Ethernet>::Observer,
    private Concurrent_Observer<Ethernet::Buffer, Ethernet::Protocol> {

public:
    typedef Ethernet::Protocol Protocol;
    typedef Ethernet::Buffer Buffer;
    typedef Ethernet::Address Address;
    typedef Data_Observer<Buffer, Protocol> Observer;
    typedef Data_Observed<Buffer, Protocol> Observed;

public:

    class Header
    {
        public:
            Header() {}
            //Header(from, to, ack):  {}
            
            

        protected:
            unsigned char  _from;
            unsigned char  _to;
            bool _ack;
    }
    
    class Packet
    {
        public:
            Packet(){}
            //Packet(Data data):  {}
            
            template<typename T>
            T * data() { return reinterpret_cast<T *>(&_data); }

        private:
            Data _data;
    }

    class Address
    {
        public:
            Address() {}
            //Address(mac, port):  {}

            Local local() const { return 0; }
        
        private:
            Port _port;
            //mac
    
    };

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
        Buffer * buff = updated();
        memcpy(data, buff->frame()->data<char>(), size);
        _nic->free(buff);
        return size;
    }

    void update(Observed *obs, const Ethernet::Protocol & prot, Buffer * buf) {
        Concurrent_Observer<Observer::Observed_Data, Protocol>::update(prot, buf);
    }

protected:

    NIC<Ethernet> * _nic;

};

__END_SYS

#endif
