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

    char data[22];
    char * text = "my text to say hello\n";

    if(self[5] % 2) { // sender
        Delay (5000000);
        cout << " Sending: " << text;
        sp->send(Simple_Protocol::Address::BROADCAST, 99, text, 22);
    } else { // receiver
        sp->receive(99, data, 22);
        cout << "  Data: " << data;
    }
}
