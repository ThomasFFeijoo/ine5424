#include <time.h>

#include <machine/nic.h>

#include <process.h>
#include <communicator.h>
#include <synchronizer.h>


using namespace EPOS;
OStream cout;

//ping test usado como exemplo
int main()
{
    cout << "TCP Test" << endl;
    
    char data[6];
    Link<TCP> * com;

    IP * ip = IP::get_by_nic(0);
    if(ip->address()[3] % 2) { // sender
        cout << "Sender:" << endl;
        data[0] = '0';
        data[1] = '1';
        data[2] = '2';

        IP::Address peer_ip = ip->address();
        peer_ip[3]--;
        com = new Link<TCP>(8000, Link<TCP>::Address(peer_ip, TCP::Port(8000))); // connect
        int sent = com->write(&data, sizeof(data));
        cout << "SENT:" << data << endl;
    } else { // receiver
        cout << "Receiver:" << endl;

        IP::Address peer_ip = ip->address();
        peer_ip[3]++;

        com = new Link<TCP>(TCP::Port(8000)); // listen
        com->read(&data, sizeof(data));
        cout << "RECEIVED " << data << endl;
    }
    delete com;
    return 0;
}