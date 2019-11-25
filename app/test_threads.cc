#include <machine/nic.h>
#include <time.h>
#include <network/simple_protocol/simple_protocol.h>

using namespace EPOS;

OStream cout;

Simple_Protocol * sp;
Simple_Protocol::Address self;
char data[Traits<Simple_Protocol>::MTU];

int sender(int port)
{
  Simple_Protocol::Address other = self;
  other[5]--;
  char * text = (char*) "my text to say hello\n";
  cout << "SENDING " << text << endl;
  cout << "Enviado para a porta " << port << endl;
  sp->send(other, port, text, Traits<Simple_Protocol>::MTU);
}

int receiver(int port)
{
  sp->receive(port, data, Traits<Simple_Protocol>::MTU);
  cout << "Recebido na porta: " << port << endl;
  cout << " Data: " << data << endl;
}


int main()
{
    sp = new Simple_Protocol();
    self = sp->address();

    cout << "Threads Test " << endl;
    cout << "  MAC: " << self << endl;

    if(self[5] % 2) { // sender
        Delay (5000000);
        new Thread(&sender,10);
        new Thread(&sender,20);
    } else { // receiver
        new Thread(&receiver,10);
        new Thread(&receiver,20);
    }
}
