#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol/simple_protocol.h>
#include <machine/uart.h>

using namespace EPOS;

OStream cout;
Simple_Protocol * sp;

//int uart_thread() {
//    sp->start_uart();
//}

int main() {
    cout << "Test uart" << endl;

    sp = new Simple_Protocol();
    Simple_Protocol::Address self = sp->address();
    char * text = (char*) "my text to say hello\n";
    char data[Traits<Simple_Protocol>::MTU];
    if(self[5] % 2) { // sender
        Delay(5000000);
        //new Thread(&uart_thread);

        Simple_Protocol::Address other = self;
        other[5]--;
        sp->send(other, 99, text, Traits<Simple_Protocol>::MTU);
        sp->send(other, 98, text, Traits<Simple_Protocol>::MTU);
    } else { // receiver
        sp->receive(99, data, Traits<Simple_Protocol>::MTU);
        sp->receive(98, data, Traits<Simple_Protocol>::MTU);
    }
}
