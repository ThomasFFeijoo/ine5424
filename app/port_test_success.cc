#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol/simple_protocol.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "Port Test Success" << endl;

    Simple_Protocol * sp = new Simple_Protocol();

    Simple_Protocol::Address self = sp->address();

    cout << "  MAC: " << self << endl;

    char data[Traits<Simple_Protocol>::MTU];
    char * text = (char*) "my text to say hello\n";

    if(self[5] % 2) { // sender
        Delay (5000000);
        sp->master(true);
        cout << "  Sending: " << text;

        Simple_Protocol::Address other = self;
        other[5]--;
        cout << "  To: " << other << "\n";

        char * result = sp->text_from_result_code(sp->send(other, 99, text, Traits<Simple_Protocol>::MTU, Simple_Protocol::SYNC_TEMP_MSG));
        cout << "  " << result << "\n";
    } else { // receiver
        char * result = sp->text_from_result_code(sp->receive(99, data, Traits<Simple_Protocol>::MTU));
        cout << "  " << result << "\n";
        cout << "  Data: " << data;
    }
}
