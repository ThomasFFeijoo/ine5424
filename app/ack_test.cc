using namespace EPOS;

OStream cout;

int main()
{
    cout << Traits<Simple_Protocol>::TIMEOUT << endl;
    // TODO: renomear aqui e no traits se for usar esse
    cout << Traits<Simple_Protocol>::MY_TIMEOUT << endl;
}
