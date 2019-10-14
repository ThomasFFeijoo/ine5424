#include <machine/nic.h>
#include <communicator.h>
#include <process.h>
#include <time.h>

using namespace EPOS;

OStream cout;

int fib(unsigned int n) {
    if (n <= 1) {
        return 1;
    } else {
        return fib(n-1) + fib(n-2);
    }
}

int dma_cpu_test(void) {
    int f = fib(40);
    return 0;
}

int dma_network_test(void) {
    cout << "Network Timing Test" << endl;

    NIC<Ethernet> * nic = Traits<Ethernet>::DEVICES::Get<0>::Result::get(0);

    NIC<Ethernet>::Address src, dst;
    NIC<Ethernet>::Protocol prot;
    char data[nic->mtu()];

    NIC<Ethernet>::Address self = nic->address();
    cout << "  MAC: " << self << endl;

    if(self[5] % 2) { // sender
        Delay (5000000);

        for(int i = 0; i < 100; i++) {
            memset(data, '0' + i, nic->mtu());
            data[nic->mtu() - 1] = '\n';
            nic->send(nic->broadcast(), 0x8888, data, nic->mtu());
        }
    } else { // receiver
        for(int i = 0; i < 100; i++) {
           nic->receive(&src, &prot, data, nic->mtu());
           cout << "  Data: " << data;
        }
    }
}

Thread *cpu_t;
Thread *nic_t;

int main()
{
    cout << "Test DMA Begin" << endl;
    cpu_t = new Thread(&dma_cpu_test);
    nic_t = new Thread(&dma_network_test);
    cpu_t->join();
    nic_t->join();
    cout << "Test DMA End" << endl;
}