// EPOS NIC Test Programs

#include <utility/ostream.h>

using namespace EPOS;

OStream cout;

//demora cerca de 12 segundos
int fib(unsigned int n) {
    if (n <= 1) {
        return 1;
    } else {
        return fib(n-1) + fib(n-2);
    }
}


int main()
{
    cout << "CPU Timing Test" << endl;
    int f = fib(40);
    cout << "END TIMER" << endl;
    return 0;

}