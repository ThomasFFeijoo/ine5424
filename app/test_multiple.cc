#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol.h>

using namespace EPOS;

OStream cout;

Simple_Protocol * sp;
Simple_Protocol::Address self;
char data[Traits<Simple_Protocol>::MTU];

int sender()
{
    int port = 10;
    Simple_Protocol::Address other = self;
    other[5]--;
    char * text = (char*) "my text to say hello\n";
    for(int port = 10; port < 15; port++) {
      cout << "SENDING " << text << endl;
      cout << "Enviado para a porta " << port << endl;
      sp->send(other, port, text, Traits<Simple_Protocol>::MTU);
    }
}

int receiver()
{
    for(int port = 10; port < 15; port++) {
        sp->receive(port, data, Traits<Simple_Protocol>::MTU);
        cout << "Recebido na porta: " << port << endl;
        cout << " Data: " << data << endl;

        }
}


int main()
{
    sp = new Simple_Protocol();
    self = sp->address();

    cout << "  MAC: " << self << endl;

    if(self[5] % 2) { // sender
        Delay (5000000);
        new Thread(&sender);
    } else { // receiver
        new Thread(&receiver);
    }
}
