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

    char data[Traits<Simple_Protocol>::MTU];
    char * text = (char*) "my text to say hello\n";

    if(self[5] % 2) { // sender
        Delay (5000000);
        Simple_Protocol::Address other = self;
        other[5]--;
        cout << "  To: " << other << "\n";
        for(int i = 5; i < 6; i++) {
            cout << " Sending: " << text;
            sp->send(other, i, text, Traits<Simple_Protocol>::MTU);
        }
    } else { // receiver
        for(int i = 5; i < 6; i++) {
           sp->receive(i, data, Traits<Simple_Protocol>::MTU);
           cout << "  Data: " << data;
        }
    }
}
