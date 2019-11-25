#ifndef __simple_protocol_h
#define __simple_protocol_h

#include <system/config.h>
#include <machine/nic.h>
#include <synchronizer.h>
#include <time.h>
#include <machine/uart.h>
#include <utility/external_libs.h>
#include <utility/math.h>
#include <utility/random.h>

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
        _nodo_position = Nodo_Position();
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
            db<Observeds>(WRN) << "TERMINA UART" << endl;
            split_nmea_message();
            db<Observeds>(WRN) << "TERMINA SPLIT" << endl;
            convert_nmea_values();
            db<Observeds>(WRN) << "TERMINA CONVERT" << endl;
        }

        Semaphore sem(0);

        int id = get_current_sender_id() ++;
        int timestamp = Alarm::elapsed();
        Package *package = new Package(address(), port, timestamp, data, id, _nodo_position.x(), _nodo_position.y(), _nodo_position._z);

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
                sync_location(receive_package->header().coord_x(), receive_package->header().coord_y());
            }

            memcpy(data, receive_package->data<char>(), size);

            char * ack = (char*) "ack";
            int timestamp = Alarm::elapsed();
            Package *ack_package = new Package(address(), port, timestamp, ack, receive_package->id(), _nodo_position.x(), _nodo_position.y(), _nodo_position._z);
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

    // time sync data
    int _t1 = -1;
    int _t2 = -1;

    double static constexpr a = 6378137.0; // WGS-84 semi-major axis
    double static constexpr  e2 = 6.6943799901377997e-3; // WGS-84 first eccentricity squared
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
            Helper helper = Helper();
            switch (id) {
            case 1:
                _timestamp = helper.atof(value);
                break;
            case 2:
                db<Observeds>(WRN) << "latitude value: " << value << endl;
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

    struct Nodo_Position {
        double _x;
        bool random_x = true;
        double _y;
        bool random_y = true;
        double _z; // z don't have random because isn't used

        double x() {
            return random_x ? 1 + Random::random() % 100 : _x;
        }

        void x(double x) {
            random_x = false;
            _x = x;
        }

        double y() {
            return random_y ? 1 + Random::random() % 100 : _y;
        }

        void y(double y) {
            random_y = false;
            _y = y;
        }

        Nodo_Position() {}
    };

    Nodo_Position _nodo_position;

    // spatial sync data
    // we don't work with negative position
    double _received_x = -1;
    double _received_y = -1;

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
                   char value[12] = {0};
                   int k = 0;
                   while (nmea_message[j] != ',') {
                       value[k] = nmea_message[j];
                       j++;
                       k++;
                   }
                   main_data_nmea.handle_value(delimiter_number, value);

                   i = j - 1;
            }
        }
        _main_data_nmea = main_data_nmea;
    }

    void convert_nmea_values() {

        if (_main_data_nmea._latitude_orientation == 'S') {
            _main_data_nmea._latitude *= -1.0;
        }

        if (_main_data_nmea._longitude_orientation == 'W') {
            _main_data_nmea._longitude *= -1.0;
        }

        Helper helper = Helper();

        double lat_radiano = helper.deg2rand(_main_data_nmea._latitude);
        double lon_radiano = helper.deg2rand(_main_data_nmea._longitude);

        double sin_lat = helper.sin(lat_radiano);
        double sin_lon = helper.sin(lon_radiano);
        double cos_lat = helper.cos(lat_radiano);
        double cos_lon = helper.cos(lon_radiano);

        double sqrt = helper.find_sqrt(1 - e2 * sin_lat * sin_lat);
        double n = a / sqrt;

        _nodo_position.x((n + _main_data_nmea._altitude) * cos_lat * cos_lon);
        _nodo_position.y((n + _main_data_nmea._altitude) * cos_lat * sin_lon);
        _nodo_position._z = (n * (1 - e2) + _main_data_nmea._altitude) * sin_lat;


        //LOOOOOOG
        db<Observeds>(WRN) << "latitude: " << _main_data_nmea._latitude << endl;
        db<Observeds>(WRN) << "longitude: " << _main_data_nmea._longitude << endl;
        db<Observeds>(WRN) << "lat_radiano: " << lat_radiano << endl;
        db<Observeds>(WRN) << "lon_radiano: " << lon_radiano << endl;
        db<Observeds>(WRN) << "sin_lat: " << sin_lat << endl;
        db<Observeds>(WRN) << "sin_lon: " << sin_lon << endl;
        db<Observeds>(WRN) << "cos_lat: " << cos_lat << endl;
        db<Observeds>(WRN) << "cos_lon: " << cos_lon << endl;
        db<Observeds>(WRN) << "sqrt: " << sqrt << endl;
        db<Observeds>(WRN) << "n: " << n << endl;
        db<Observeds>(WRN) << "x: " << _nodo_position.x() << endl;
        db<Observeds>(WRN) << "y: " << _nodo_position.y() << endl;
        db<Observeds>(WRN) << "z: " << _nodo_position._z << endl;
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

    void sync_location(double x, double y) {
        if (_received_x < 0 && _received_y < 0) {
            _received_x = x;
            _received_y = y;
        } else {
            Helper helper = Helper();

            double r1 = 1 + Random::random() % 100;
            double r2 = 1 + Random::random() % 100;

            // distance between the two known points
            double x_part = x - _received_x;
            double y_part = y - _received_y;
            double u = helper.find_sqrt(pow(x_part, 2) + pow(y_part, 2));

            double r1_2 = pow(r1, 2);

            _nodo_position.x((r1_2 - pow(r2, 2) + pow(u, 2)) / (2 * u));
            _nodo_position.y(helper.find_sqrt(r1_2 - pow(_nodo_position.x(), 2)));
            _nodo_position._z = 0;

            // its log time
            db<Observeds>(INF) << "  _received_x " << _received_x << endl;
            db<Observeds>(INF) << "  _received_y " << _received_y << endl;
            db<Observeds>(INF) << "  x " << x << endl;
            db<Observeds>(INF) << "  y " << y << endl;
            db<Observeds>(INF) << "  r1 " << r1 << endl;
            db<Observeds>(INF) << "  r2 " << r2 << endl;
            db<Observeds>(INF) << "  u " << u << endl;
            db<Observeds>(INF) << "  _nodo_position._x " << _nodo_position.x() << endl;
            db<Observeds>(INF) << "  _nodo_position._y " << _nodo_position.y() << endl;
        }
    }

public:

    class Header {

    private:

        Address _from;
        unsigned int _port;
        int _timestamp;
        double _x;
        double _y;
        double _z;

    public:

        Header() {}

        Header(Address from, unsigned int port, int timestamp, double x, double y, double z):
            _from(from), _port(port), _timestamp(timestamp), _x(x), _y(y), _z(z) {}

        Address from() {
            return _from;
        }

        unsigned int port() {
            return _port;
        }

        int timestamp() {
            return _timestamp;
        }

        double coord_x() {
            return _x;
        }

        double coord_y() {
            return _x;
        }

        double coord_z() {
            return _x;
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

        Package(Address from, unsigned int port, int timestamp, void * data, int id, double x, double y, double z):
            _data(data), _id(id) {
                _header = Header(from, port, timestamp, x, y, z);
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
