// EPOS PC UART Mediator Test Program

#include <utility/ostream.h>
#include <machine/uart.h>

using namespace EPOS;

int main()
{
    OStream cout;

    cout << "UART Socket test\n" << endl;

    UART uart(0, 115200, 8, 0, 1); // using 0 for network, 1 is default for console (-serial mon:stdio)

    cout << "Loopback transmission test (conf = 115200 8N1):";
    uart.loopback(true);

    for(int i = 0; i < 10; i++) {
        int c = uart.get(); // getting ASCII input as int
        cout << "received (" << c << ")" << endl;
        uart.put(i + 48); // sending "0", "1", "2" in ASCII
        cout  << "sent (" << i << ")" << endl;
    }
    cout << "end!" << endl;

    return 0;
}