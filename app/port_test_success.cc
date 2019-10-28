#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "Port Test Success" << endl;

    Simple_Protocol * sp = new Simple_Protocol();

    Simple_Protocol::Address self = sp->address();

    cout << "  MAC: " << self << endl;

    // XXX: mudar para constante
    char data[1500];
    char * text = (char*) "my text to say hello\n";

    if(self[5] % 2) { // sender
        Delay (5000000);
        cout << "  Sending: " << text;

        Simple_Protocol::Address other = self;
        other[5]--;
        cout << "  To: " << other;

        sp->send(other, 99, text, 1500);
    } else { // receiver
        sp->receive(99, data, 1500);
        cout << "  Data: " << data;
    }
}
