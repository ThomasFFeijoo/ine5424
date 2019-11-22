#ifndef __simple_protocol_h
#define __simple_protocol_h

#include <system/config.h>
#include <machine/nic.h>
#include <synchronizer.h>
#include <time.h>
#include <machine/uart.h>
#include <utility/ostream.h>

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
        if (_allow_sync) {
            start_uart();
            // TODO: talvez chamar esquema da uart aqui
            split_nmea_message();
            // TODO: realizar conversão de latitude, longitude e altura para x, y e z por aqui ou na uart
        }

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
                sync_location();
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

    // TODO: verificar problema de 4 qemus vs qemu
    void start_uart() {
        UART uart(1, 115200, 8, 0, 1);
        uart.loopback(false);

        char c;
        bool is_checksum = false;
        bool get_data = true;
        int checksum_length = 2;
        int i = 0;
        while (get_data) {
            c = uart.get();
            nmea_message[i] = c;
            i++;

            // things to control while flow
            if (is_checksum) {
                checksum_length -= 1;
                get_data = checksum_length != 0;
            } else {
                is_checksum = c == '*';
            }

            // it's log time
            db<Observeds>(WRN) << "uart.get(" << c << ")" << endl;
        }
        db<Observeds>(WRN) << "received: " << nmea_message  << endl;
    }

private:

    Ordered_List<Package_Semaphore, int> _controller_structs;
    bool _allow_sync = true;

    int _t1 = -1;
    int _t2 = -1;

    int static const MAX_LENGTH = 82; // 82 is the max length of nmea message by wikipedia

    char nmea_message[MAX_LENGTH];

    struct Main_Data_NMEA {
        double _timestamp;
        double _latitude;
        char _latitude_orientation;
        double _longitude;
        char _longitude_orientation;
        double _altitude;

        Main_Data_NMEA() {}

        void handle_value(int id, char value[]) {
            OStream helper = OStream();
            switch (id) {
            case 1:
                _timestamp = helper.atof(value);
                break;
            case 2:
                _latitude = helper.atof(value);
                break;
            case 3:
                _latitude_orientation = value[0];
                break;
            case 4:
                _longitude = helper.atof(value);
                break;
            case 5:
                _longitude_orientation = value[0];
                break;
            case 9:
                _altitude = helper.atof(value);
                break;
            default:
                break;
            }
        }
    };

    Main_Data_NMEA _main_data_nmea;

private:

    void split_nmea_message() {
        Main_Data_NMEA main_data_nmea;
        int delimiter_number = 0;
        for (int i = 0; i <= MAX_LENGTH; i++) {
            if (nmea_message[i] == ',') {
                delimiter_number++;
            }

            if (delimiter_number == 1 || // timestamp
                delimiter_number == 2 || // latitude
                delimiter_number == 3 || // latitude orientation
                delimiter_number == 4 || // longitude
                delimiter_number == 5 || // longitude orientation
                delimiter_number == 9    // altitude
               ) {
                   int j = i + 1;
                   char value[12];
                   int k = 0;
                   while (nmea_message[j] != ',') {
                       value[k] = nmea_message[j];
                       j++;
                       k++;
                   }
                   main_data_nmea.handle_value(delimiter_number, value);
            }
        }
        _main_data_nmea = main_data_nmea;
    }

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

    void sync_location() {
        // TODO: verificar necessidade de add argumento
        // TODO: realizar trilateração
        // TODO: gerar valores aleatórios dentro do range permitido para distância
        // TODO: verificar se a distância deve ser uma variável global
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
