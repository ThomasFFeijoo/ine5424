// EPOS NIC Test Programs

#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "NIC Test" << endl;
    Simple_Protocol * sp = new Simple_Protocol();

    Simple_Protocol::Address self = sp->address();

    cout << "  MAC: " << self << endl;

    char data[5];
    
    if(self[5] % 2) { // sender
        Delay (5000000);

        for(int i = 0; i < 10; i++) {
            memset(data, '0' + i, 5);
            data[4] = '\n';
            cout << " Sending: " << data;
            sp->send(Simple_Protocol::Address::BROADCAST, data, 5);
        }
    } else { // receiver
        for(int i = 0; i < 10; i++) {
           sp->receive(data, 5);
           cout << "  Data: " << data;
        }
    }
}
