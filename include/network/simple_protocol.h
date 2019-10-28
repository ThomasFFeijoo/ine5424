#ifndef __simple_protocol_h
#define __simple_protocol_h

#include <system/config.h>
#include <machine/nic.h>
#include <synchronizer.h>
#include <time.h>

__BEGIN_SYS

class Simple_Protocol:
    private NIC<Ethernet>::Observer,
    private Concurrent_Observer<Ethernet::Buffer, Ethernet::Protocol> {

protected:

    NIC<Ethernet> * _nic;

public:
    typedef Ethernet::Protocol Protocol;
    typedef Ethernet::Buffer Buffer;
    typedef Ethernet::Address Address;
    typedef Data_Observer<Buffer, Protocol> Observer;
    typedef Data_Observed<Buffer, Protocol> Observed;

public:

    Simple_Protocol(unsigned int nic = 0) :
            _nic(Traits<Ethernet>::DEVICES::Get<0>::Result::get(nic))
    {
        _nic->attach(this, Ethernet::PROTO_SP);
    }

    const Address & address() {
        return _nic->address();
    }

    void send(const Address & dst, unsigned int port, void * data, unsigned int size) {
        Semaphore sem(0);
        bool receive_ack = false;

        Package *package = new Package(address(), port, data, &receive_ack, &sem);

        for (unsigned int i = 0; (i < Traits<Simple_Protocol>::SP_RETRIES) && !receive_ack; i++) {
            _nic->send(dst, Ethernet::PROTO_SP, package, size);

            Semaphore_Handler handler(&sem);
            Alarm alarm(Traits<Simple_Protocol>::SP_TIMEOUT * 1000000, &handler, 1);
            sem.p();
        }

        if (receive_ack) {
            db<Observeds>(WRN) << "Mensagem confirmada na porta " << port << endl;
        } else  {
            db<Observeds>(WRN) << "Falha ao enviar mensagem na porta " << port << endl;
        }
    }

    void receive(unsigned int port, void * data, unsigned int size) {
        Buffer * buff = updated();
        Package *receive_package = reinterpret_cast<Package*>(buff->frame()->data<char>());

        if (port == receive_package->port()) {
            memcpy(data, receive_package->data<char>(), size);

            char * ack = (char*) "ack";
            Package *ack_package = new Package(address(), port, ack, receive_package->receive_ack(), receive_package->semaphore());
            ack_package->ack(true);
            _nic->send(receive_package->from(), Ethernet::PROTO_SP, ack_package, size);
        } else {
            db<Observeds>(WRN) << "Pacote recebido na porta " << receive_package->port() << ", mas era esperado na porta " << port << endl;
        }

        _nic->free(buff);
    }

    void update(Observed *obs, const Ethernet::Protocol & prot, Buffer * buf) {
        Package *package = reinterpret_cast<Package*>(buf->frame()->data<char>());
        if (package->ack()) {
            db<Observeds>(WRN) << "ack no update" << endl;
            package->receive_ack_to_write() = true;
            package->semaphore()->v();
        }

        Concurrent_Observer<Observer::Observed_Data, Protocol>::update(prot, buf);
    }

public:

    class Package {

    private:

        Address _from;
        unsigned int _port;
        void * _data;
        // define if the package is an ack
        bool _ack = false;
        // define if the package received is an ack, used to control retries of send
        // if _ack is true, then this attribute has the value from a previous package that wasn't a representation of ack
        bool* _receive_ack;
        Semaphore * _semaphore;

    public:

        Package(Address from, unsigned int port, void * data, bool* receive_ack, Semaphore * semaphore):
            _from(from), _data(data), _port(port), _receive_ack(receive_ack), _semaphore(semaphore) {}

        Address from() {
            return _from;
        }

        unsigned int port() {
            return _port;
        }

        template<typename T>
        T * data() {
            return reinterpret_cast<T *>(_data);
        }

        bool ack() {
            return _ack;
        }

        bool * receive_ack() {
            return _receive_ack;
        }

        Semaphore * semaphore() {
            return _semaphore;
        }

        void ack(bool ack) {
            _ack = ack;
        }

        bool & receive_ack_to_write() {
            return *_receive_ack;
        }

    };

};

__END_SYS

#endif
