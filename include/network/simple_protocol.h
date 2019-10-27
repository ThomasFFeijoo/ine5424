#ifndef __simple_protocol_h
#define __simple_protocol_h

#include <system/config.h>
#include <machine/nic.h>
#include <synchronizer.h>

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
        bool ack = false;

        Package *package = new Package(port, data, &ack, &sem);

        // TODO: usar traits correto aqui
        for (unsigned int i = 0; (i < 3) && !ack; i++) {
            _nic->send(dst, Ethernet::PROTO_SP, package, size);

            Semaphore_Handler handler(&sem);
            // TODO usar traits correto aqui
            Alarm alarm(Traits<Network>::TIMEOUT * 1000000, &handler, 1);
            sem.p();
        }

        if (ack) {
            db<Observeds>(WRN) << "Mensagem confirmada na porta " << port << endl;
        } else  {
            db<Observeds>(WRN) << "Falha ao enviar mensagem na porta " << port << endl;
        }
    }

    void receive(unsigned int port, void * data, unsigned int size) {
        Buffer * buff = updated();
        Package *package = reinterpret_cast<Package*>(buff->frame()->data<char>());

        // TODO: mandar ack

        if (port == package->port()) {
            memcpy(data, package->data<char>(), size);
        } else {
            db<Observeds>(WRN) << "Pacote recebido na porta " << package->port() << ", mas era esperado na porta " << port << endl;
        }

        _nic->free(buff);
    }

    void update(Observed *obs, const Ethernet::Protocol & prot, Buffer * buf) {
        Package *package = reinterpret_cast<Package*>(buff->frame()->data<char>());
        if (package->ack()) {
            // TODO: mudar ack
            db<Observeds>(WRN) << "ack no update" << endl;
        }

        Concurrent_Observer<Observer::Observed_Data, Protocol>::update(prot, buf);
    }

public:

    class Package {

    private:

        void * _data;
        unsigned int _port;
        bool* _ack;
        Semaphore * _semaphore;

    public:

        Package(unsigned int port, void * data, bool* ack, Semaphore * semaphore):
            _data(data), _port(port), _ack(ack), _semaphore(semaphore) {}

        unsigned int port() {
            return _port;
        }

        template<typename T>
        T * data() {
            return reinterpret_cast<T *>(_data);
        }

        bool* ack() {
            return _ack;
        }

    };

};

__END_SYS

#endif
