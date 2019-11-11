#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol.h>

using namespace EPOS;

OStream cout;


int main()
{
    cout << "Test sync" << endl;

    int second = 1000000;
    Simple_Protocol * sp = new Simple_Protocol();

    Simple_Protocol::Address self = sp->address();

    cout << "  MAC: " << self << endl;

    char data[Traits<Simple_Protocol>::MTU];
    char * text = (char*) "my text to say hello\n";

    if(self[5] % 2) { // sender
        Delay (5 * second);
        sp->master(true);
        cout << "  Sending: " << text;

        Simple_Protocol::Address other = self;
        other[5]--;
        cout << "  To: " << other << "\n";
        cout << "  Time before send 1 : " << sp->get_time() << "\n";
        char * result_sync = sp->text_from_result_code(sp->send(other, 99, text, Traits<Simple_Protocol>::MTU, Simple_Protocol::SYNC_TEMP_MSG));
        Delay(2 * second);
        cout << "  Time before send 2 : " << sp->get_time() << "\n";
        char * result_follow = sp->text_from_result_code(sp->send(other, 99, text, Traits<Simple_Protocol>::MTU, Simple_Protocol::FOLLOW_UP_SYNC_TEMP_MSG));
        cout << "  Time after sends: " << sp->get_time() << "\n";
    } else { // receiver
        sp->reset_elapsed();
        cout << "  Time before receive 1 : " << sp->get_time() << "\n";
        char * result = sp->text_from_result_code(sp->receive(99, data, Traits<Simple_Protocol>::MTU));

        char * result_sync = sp->text_from_result_code(sp->receive(99, data, Traits<Simple_Protocol>::MTU));

        cout << "  Time after receives: " << sp->get_time() << "\n";
    }
}
