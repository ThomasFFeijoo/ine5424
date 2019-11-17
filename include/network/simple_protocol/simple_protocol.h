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

    result_code send(const Address & dst, unsigned int port, void * data, unsigned int size) {
        Semaphore sem(0);

        int id = get_current_sender_id() ++;
        int timestamp = Alarm::elapsed();
        Package *package = new Package(address(), port, timestamp, data, id);

        Package_Semaphore * ps = new Package_Semaphore(id, false, &sem);
        List_Package_Semaphore * e = new List_Package_Semaphore(ps, id);
        _controller_structs.insert(e);

        for (unsigned int i = 0; (i < Traits<Simple_Protocol>::SP_RETRIES) && !ps->_ack; i++) {
            db<Observeds>(INF) << "Tentativa de envio numero: " << i + 1 << endl;
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
        if (port == receive_package->header().port()) {
            if (_allow_sync) {
                sync_time(receive_package->header().timestamp());
            }

            memcpy(data, receive_package->data<char>(), size);

            char * ack = (char*) "ack";
            int timestamp = Alarm::elapsed();
            Package *ack_package = new Package(address(), port, timestamp, ack, receive_package->id());
            ack_package->ack(true);

            _nic->send(receive_package->header().from(), Ethernet::PROTO_SP, ack_package, size);
            code = SUCCESS_ACK;
        } else {
            code = ERROR_RECEIVE_PORT;
        }
        _nic->free(buff);
        return code;
    }

    void update(Observed *obs, const Ethernet::Protocol & prot, Buffer * buf) {
        Package *package = reinterpret_cast<Package*>(buf->frame()->data<char>());
        if (package->ack()) {
            List_Package_Semaphore * lps = _controller_structs.search_rank(package->id());
            if(lps) {
                db<Observeds>(INF) << "ack no update do sender" << endl;
                Package_Semaphore * ps = lps->object();
                ps->_ack = true;
                ps->_sem->v();
                _controller_structs.remove_rank(package->id());
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

    // to help the tests
    void reset_elapsed() {
        Alarm::elapsed() = 1000;
    }

    void allow_sync(bool allow_sync) {
        _allow_sync = allow_sync;
    }

private:

    Ordered_List<Package_Semaphore, int> _controller_structs;
    bool _allow_sync = true;

    int _t1 = -1;
    int _t2 = -1;

private:

    void sync_time(int timestamp) {
        if (_t1 < 0 && _t2 < 0) {
            _t1 = timestamp;
            _t2 = Alarm::elapsed();
        } else {
            // timestamp here is t4
            int t3 = Alarm::elapsed();
            int pd = ((_t2 - _t1) + (timestamp - t3)) / 2;
            int offset = (_t2 - _t1) - pd;
            Alarm::elapsed() = t3 - offset;

            // its erase time
            _t1 = -1;
            _t2 = -1;

            // its log time
            db<Observeds>(INF) << "  _t1 " << _t1 << endl;
            db<Observeds>(INF) << "  _t2 " << _t2 << endl;
            db<Observeds>(INF) << "  timestamp " << timestamp << endl;
            db<Observeds>(INF) << "  t3 " << t3 << endl;
            db<Observeds>(INF) << "  pd " << pd << endl;
            db<Observeds>(INF) << "  offset " << offset << endl;
            db<Observeds>(INF) << "  result " << t3 - offset << endl;
        }
    }

public:

    class Header {

    private:

        Address _from;
        unsigned int _port;
        int _timestamp;
        // TODO: populate this values
        double _x;
        double _y;
        double _z;

    public:

        Header() {}

        Header(Address from, unsigned int port, int timestamp):
            _from(from), _port(port), _timestamp(timestamp) {}

        Address from() {
            return _from;
        }

        unsigned int port() {
            return _port;
        }

        int timestamp() {
            return _timestamp;
        }

    };

public:

    class Package {

    private:

        Header _header;
        void * _data;
        int _id;
        // define if the package is a ack
        bool _ack = false;

    public:

        Package(Address from, unsigned int port, int timestamp, void * data, int id):
            _data(data), _id(id) {
                _header = Header(from, port, timestamp);
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

        bool ack() {
            return _ack;
        }

        void ack(bool ack) {
            _ack = ack;
        }

    };

};



__END_SYS

#endif
