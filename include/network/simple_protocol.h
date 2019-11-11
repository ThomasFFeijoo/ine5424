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

    static const char NORMAL_MSG    = '0';
    static const char ACK_MSG       = '1';
    static const char SYNC_TEMP_MSG = '2';
    static const char FOLLOW_UP_SYNC_TEMP_MSG = '3';

    enum result_code {
        SUCCESS_SEND       = 1,
        SUCCESS_ACK        = 2,
        ERROR_SEND         = 10,
        ERROR_RECEIVE_PORT = 11,
        ERROR_SLAVE_WANNA_SYNC = 12,
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
            case ERROR_SLAVE_WANNA_SYNC:
                return (char *) "Escravo não pode enviar mensagem de sincronização de tempo";
            default:
                return (char *) "Código de resultado desconhecido";
        }
    }

    struct Package_Semaphore {
        int _package_id;
        bool _ack;
        Semaphore *_sem;

        Package_Semaphore(int package_id,  bool ack, Semaphore *sem):
            _package_id(package_id), _ack(ack), _sem(sem){}
    };
    typedef List_Elements::Doubly_Linked_Ordered<Package_Semaphore, int> List_Package_Semaphore;

public:

    Simple_Protocol(unsigned int nic = 0) :
            _nic(Traits<Ethernet>::DEVICES::Get<0>::Result::get(nic))
    {
        _nic->attach(this, Ethernet::PROTO_SP);
    }

    const Address & address() {
        return _nic->address();
    }

    // to help the tests
    int get_time() {
        return Alarm::elapsed();
    }

    // only for backward compatibility
    result_code send(const Address & dst, unsigned int port, void * data, unsigned int size) {
        return send(dst, port, data, size, NORMAL_MSG);
    }

    result_code send(const Address & dst, unsigned int port, void * data, unsigned int size, char msg_type) {
        if (is_slave() && is_sync_type_msg(msg_type)) {
            return ERROR_SLAVE_WANNA_SYNC;
        }

        Semaphore sem(0);

        int id = get_current_sender_id() ++;
        int timestamp = Alarm::elapsed();
        Package *package = new Package(address(), port, timestamp, msg_type, data, id);

        Package_Semaphore * ps = new Package_Semaphore(id, false, &sem);
        List_Package_Semaphore * e = new List_Package_Semaphore(ps, id);
        _semaphores.insert(e);

        for (unsigned int i = 0; (i < Traits<Simple_Protocol>::SP_RETRIES) && !ps->_ack; i++) {
            db<Observeds>(WRN) << "Tentativa de envio numero: " << i + 1 << endl;
            _nic->send(dst, Ethernet::PROTO_SP, package, size);

            Semaphore_Handler handler(&sem);
            Alarm alarm(Traits<Simple_Protocol>::SP_TIMEOUT * 1000000, &handler, 1);
            sem.p();
        }
        return ps->_ack ? SUCCESS_SEND : ERROR_SEND;
    }

    result_code receive(unsigned int port, void * data, unsigned int size) {
        result_code code;
        Buffer * buff = updated();
        Package *receive_package = reinterpret_cast<Package*>(buff->frame()->data<char>());
        // drop message if port 2 (only to test this scenario)
        if (receive_package->header().port() != 2) {
            if (port == receive_package->header().port()) {
                // we only make a special handling when is to sync time and the sp is slave
                if (is_slave() && is_sync_type_msg(receive_package->header().type())) {
                    sync_time(receive_package->header().type(), receive_package->header().timestamp());
                } else {
                    memcpy(data, receive_package->data<char>(), size);
                }

                char * ack = (char*) "ack";
                int timestamp = Alarm::elapsed();
                Package *ack_package = new Package(address(), port, timestamp, ACK_MSG, ack, receive_package->id());

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
        Package *package = reinterpret_cast<Package*>(buf->frame()->data<char>());
        if (package->header().type() == ACK_MSG) {
            List_Package_Semaphore * lps = _semaphores.search_rank(package->id());
            if(lps) {
                db<Observeds>(INF) << "ack no update do sender" << endl;
                Package_Semaphore * ps = lps->object();
                ps->_sem->v();
                ps->_ack = true;
                _semaphores.remove_rank(package->id());
            }
            _nic->free(buf);
        }

        Concurrent_Observer<Observer::Observed_Data, Protocol>::update(prot, buf);
    }

    static int & get_current_sender_id ()
    {
       static int current_send_id = 0;
       return current_send_id;
    }

    void master(bool master) {
        _master = master;
    }

    // to help the tests
    void reset_elapsed() {
        Alarm::elapsed() = 1000;
    }

private:

    Ordered_List<Package_Semaphore, int> _semaphores;
    // default is be a slave
    bool _master = false;

    int _t1;
    int _t2;

private:

    void sync_time(char msg_type, int timestamp) {
        if (msg_type == SYNC_TEMP_MSG) {
            _t1 = timestamp;
            _t2 = Alarm::elapsed();
        } else if (msg_type == FOLLOW_UP_SYNC_TEMP_MSG) {
            // timestamp here is t4
            int t3 = Alarm::elapsed();
            int pd = ((_t2 - _t1) + (timestamp - t3)) / 2;
            int offset = (_t2 - _t1) - pd;
            Alarm::elapsed() = t3 - offset;

            // its log time
            db<Observeds>(WRN) << "  _t1 " << _t1 << endl;
            db<Observeds>(WRN) << "  _t2 " << _t2 << endl;
            db<Observeds>(WRN) << "  timestamp " << timestamp << endl;
            db<Observeds>(WRN) << "  t3 " << t3 << endl;
            db<Observeds>(WRN) << "  pd " << pd << endl;
            db<Observeds>(WRN) << "  offset " << offset << endl;
            db<Observeds>(WRN) << "  result " << t3 - offset << endl;
        }
    }

    bool is_slave() {
        return !_master;
    }

    bool is_sync_type_msg(char msg_type) {
        return msg_type == SYNC_TEMP_MSG || msg_type == FOLLOW_UP_SYNC_TEMP_MSG;
    }

public:

    class Header {

    private:

        Address _from;
        unsigned int _port;
        int _timestamp;
        // define the type of the package
        char _type = NORMAL_MSG;

    public:

        Header() {}

        Header(Address from, unsigned int port, int timestamp, char type):
            _from(from), _port(port), _timestamp(timestamp), _type(type) {}

        Address from() {
            return _from;
        }

        unsigned int port() {
            return _port;
        }

        int timestamp() {
            return _timestamp;
        }

        char type() {
            return _type;
        }

        void type(char type) {
            _type = type;
        }

    };

public:

    class Package {

    private:

        Header _header;
        void * _data;
        int _id;

    public:

        Package(Address from, unsigned int port, int timestamp, char type, void * data, int id):
            _data(data), _id(id) {
                _header = Header(from, port, timestamp, type);
            }

        Header header() {
            return _header;
        }

        template<typename T>
        T * data() {
            return reinterpret_cast<T *>(_data);
        }

        int id() {
            return _id;
        }

    };

};



__END_SYS

#endif
