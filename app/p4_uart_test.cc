// EPOS PC UART Mediator Test Program
#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol/simple_protocol.h>
#include <machine/uart.h>

using namespace EPOS;

OStream cout;
Simple_Protocol * sp;

int uart_thread() {
    sp->start_uart();
}

int main() {
    cout << "Test uart" << endl;

    sp = new Simple_Protocol();
    Simple_Protocol::Address self = sp->address();

    if(self[5] % 2) { // sender
        Delay(5000000);
        new Thread(&uart_thread);
    } else { // receiver
    }
}
