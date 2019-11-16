#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol/simple_protocol.h>

using namespace EPOS;

OStream cout;


int main()
{
    cout << "Test slave validation added in sync" << endl;

    int second = 1000000;
    Simple_Protocol * sp = new Simple_Protocol();
    Simple_Protocol::Address self = sp->address();
    char data[Traits<Simple_Protocol>::MTU];
    char * text = (char*) "my text to say hello\n";

    if (self[5] % 2) { // sender
        Delay(5 * second);
        Simple_Protocol::Address other = self;
        other[5]--;

        char * result_sync = sp->text_from_result_code(sp->send(other, 99, text, Traits<Simple_Protocol>::MTU, Simple_Protocol::SYNC_TEMP_MSG));
        cout << "  Result code from sync send: " << result_sync << "\n";

        char * result_follow = sp->text_from_result_code(sp->send(other, 99, text, Traits<Simple_Protocol>::MTU, Simple_Protocol::FOLLOW_UP_SYNC_TEMP_MSG));
        cout << "  Result code from follow send: " << result_follow << "\n";
    } else { // receiver
        sp->receive(99, data, Traits<Simple_Protocol>::MTU);
        sp->receive(99, data, Traits<Simple_Protocol>::MTU);
    }
}
