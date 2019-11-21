// EPOS PC UART Mediator Test Program
#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol/simple_protocol.h>
#include <machine/uart.h>

using namespace EPOS;

OStream cout;

Simple_Protocol * sp;

int uart_thread()
{
    sp->start_uart();    
}

int main()
{
    sp = new Simple_Protocol();
    new Thread(&uart_thread);
}