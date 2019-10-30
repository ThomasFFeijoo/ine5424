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
        int current_sent_id = getCurrentSenderId () ++;
        db<Observeds>(WRN) << "CURRENT_ID: " << current_sent_id << endl;
        Package *package = new Package(address(), port, data, &receive_ack, &sem, current_sent_id);

        for (unsigned int i = 0; (i < Traits<Simple_Protocol>::SP_RETRIES) && !receive_ack; i++) {
            db<Observeds>(WRN) << "Tentativa de envio numero: " << i + 1 << endl;
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
            for(int i = 0; i < 2; i++){
                Buffer * buff = updated();
                Package *receive_package = reinterpret_cast<Package*>(buff->frame()->data<char>());
                
                if(getCurrentReceiverId() == receive_package->id())   // É UM RETRY
                {
                    char * ack = (char*) "ack";
                    Package *ack_package = new Package(address(), receive_package->port(), ack, receive_package->receive_ack(), receive_package->semaphore(), receive_package->id());
                    ack_package->ack(true);
                    _nic->send(receive_package->from(), Ethernet::PROTO_SP, ack_package, size);
                    _nic->free(buff);
                    i--;
                } else {
                    if (port == receive_package->port()) {
                        memcpy(data, receive_package->data<char>(), size);

                        char * ack = (char*) "ack";
                        Package *ack_package = new Package(address(), port, ack, receive_package->receive_ack(), receive_package->semaphore(), receive_package->id());
                        ack_package->ack(true);
                        if(port == 5 && i == 1) {
                            _nic->send(receive_package->from(), Ethernet::PROTO_SP, ack_package, size);
                        }
                        if(port != 5) {
                            _nic->send(receive_package->from(), Ethernet::PROTO_SP, ack_package, size);
                        }
                        
                        db<Observeds>(WRN) << "Pacote recebido na porta " << receive_package->port() << endl;
                        db<Observeds>(WRN) << "ack enviado" << endl;
                        getCurrentReceiverId () ++; //0
                        _nic->free(buff);
                        break;
                    } else {
                        db<Observeds>(WRN) << "Pacote recebido na porta " << receive_package->port() << ", mas era esperado na porta " << port << endl;
                        _nic->free(buff);
                        break;
                    }
                }

            }
            
    }

    void update(Observed *obs, const Ethernet::Protocol & prot, Buffer * buf) {
        db<Observeds>(WRN) << "update executado" << endl;
        Package *package = reinterpret_cast<Package*>(buf->frame()->data<char>());
        if (package->ack()) {
            db<Observeds>(WRN) << "ack no update do sender" << endl;
            package->receive_ack_to_write() = true;
            package->semaphore()->v();
            _nic->free(buf);
        }

        Concurrent_Observer<Observer::Observed_Data, Protocol>::update(prot, buf);
    }

    static int & getCurrentSenderId ()
    {
       static int current_send_id = 1;
       return current_send_id;
    }

    static int & getCurrentReceiverId ()
    {
       static int current_receiver_id = 0;
       return current_receiver_id;
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
        unsigned int _id;
        Semaphore * _semaphore;

    public:

        Package(Address from, unsigned int port, void * data, bool* receive_ack, Semaphore * semaphore, unsigned int id):
            _from(from), _data(data), _port(port), _receive_ack(receive_ack), _semaphore(semaphore), _id(id) {}

        Address from() {
            return _from;
        }

        unsigned int port() {
            return _port;
        }

        unsigned int id() {
            return _id;
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
