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

    enum result_code {
        SUCCESS_SEND       = 1,
        SUCCESS_ACK        = 2,
        ERROR_SEND         = 10,
        ERROR_RECEIVE_PORT = 11
    };

    char * text_from_result_code(result_code code) {
        switch (code) {
            case SUCCESS_SEND:
                return (char *) "Mensagem enviada com sucesso";
            case SUCCESS_ACK:
                return (char *) "ack enviado com sucesso";
            case ERROR_SEND:
                return (char *) "Falhar ao enviar mensagem";
            case ERROR_RECEIVE_PORT:
                return (char *) "Mensagem recebida na porta errada";
            default:
                return (char *) "Código de resultado desconhecido";
        }
    }

public:

    Simple_Protocol(unsigned int nic = 0) :
            _nic(Traits<Ethernet>::DEVICES::Get<0>::Result::get(nic))
    {
        _nic->attach(this, Ethernet::PROTO_SP);
    }

    const Address & address() {
        return _nic->address();
    }

    result_code send(const Address & dst, unsigned int port, void * data, unsigned int size) {
        Semaphore sem(0);
        bool receive_ack = false;

        Header package_header = Header(address(), port, &receive_ack, &sem);
        Package *package = new Package(package_header, data);

        for (unsigned int i = 0; (i < Traits<Simple_Protocol>::SP_RETRIES) && !receive_ack; i++) {
            db<Observeds>(INF) << "Tentativa de envio numero: " << i + 1 << endl;
            _nic->send(dst, Ethernet::PROTO_SP, package, size);

            Semaphore_Handler handler(&sem);
            Alarm alarm(Traits<Simple_Protocol>::SP_TIMEOUT * 1000000, &handler, 1);
            sem.p();
        }

        return receive_ack ? SUCCESS_SEND : ERROR_SEND;
    }

    result_code receive(unsigned int port, void * data, unsigned int size) {
        result_code code;
        Buffer * buff = updated();
        Package *receive_package = reinterpret_cast<Package*>(buff->frame()->data<char>());
        if(receive_package->header().port() != 2) { // drop message if port 2
            if (port == receive_package->header().port()) {
                memcpy(data, receive_package->data<char>(), size);

                char * ack = (char*) "ack";
                Header package_header = Header(address(), port, receive_package->header().receive_ack(), receive_package->header().semaphore());
                Package *ack_package = new Package(package_header, ack);
                ack_package->header().ack(true);
                _nic->send(receive_package->header().from(), Ethernet::PROTO_SP, ack_package, size);
                code = SUCCESS_ACK;
            } else {
                code = ERROR_RECEIVE_PORT;
            }
        }
        _nic->free(buff);
        return code;
    }

    void update(Observed *obs, const Ethernet::Protocol & prot, Buffer * buf) {
        db<Observeds>(INF) << "update executado" << endl;
        Package *package = reinterpret_cast<Package*>(buf->frame()->data<char>());
        if (package->header().ack()) {
            db<Observeds>(INF) << "ack no update do sender" << endl;
            package->header().receive_ack_to_write() = true;
            package->header().semaphore()->v();
            _nic->free(buf);
        }

        Concurrent_Observer<Observer::Observed_Data, Protocol>::update(prot, buf);
    }

public:

    class Header {

    private:

        Address _from;
        unsigned int _port;
        // define if the package is an ack
        bool _ack = false;
        // define if the package received is an ack, used to control retries of send
        // if _ack is true, then this attribute has the value from a previous package that wasn't a representation of ack
        bool* _receive_ack;
        Semaphore * _semaphore;

    public:

        Header(Address from, unsigned int port, bool* receive_ack, Semaphore * semaphore):
            _from(from), _port(port), _receive_ack(receive_ack), _semaphore(semaphore) {}

        Address from() {
            return _from;
        }

        unsigned int port() {
            return _port;
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

public:

    class Package {

    private:

        Header _header;
        void * _data;

    public:

        Package(Header header, void * data): _header(header), _data(data) {}

        Header header() {
            return _header;
        }

        template<typename T>
        T * data() {
            return reinterpret_cast<T *>(_data);
        }

    };

};

__END_SYS

#endif
