#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "Retry Test " << endl;

    Simple_Protocol * sp = new Simple_Protocol();

    Simple_Protocol::Address self = sp->address();

    cout << "  MAC: " << self << endl;

    char data[Traits<Simple_Protocol>::MTU];
    char * text = (char*) "my text to say hello\n";

    if(self[5] % 2) { // sender
        Delay (5000000);
        cout << "  Sending: " << text;

        Simple_Protocol::Address other = self;
        other[5]--;
        cout << "  To: " << other << "\n";

        sp->send(other, 2, text, Traits<Simple_Protocol>::MTU);
    } else { // receiver
        sp->receive(2, data, Traits<Simple_Protocol>::MTU);
        cout << "  Data: " << data;
    }
}
