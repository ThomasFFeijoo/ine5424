#include <network/simple_protocol.h>

#ifdef __sp__

__BEGIN_SYS

// template<unsigned int UNIT>
// My_Protocol::My_Protocol(unsigned int unit)
// :_nic(Traits<Ethernet>::DEVICES::Get<UNIT>::Result::get(unit)),
//  _address(Traits<IP>::Config<UNIT>::ADDRESS)
// {
//     db<My_Protocol>(TRC) << "My_Protocol::My_Protocol(nic=" << _nic << ") => " << this << endl;

//     _nic->attach(this, NIC<Ethernet>::PROTO_MP);

//     if(Traits<IP>::Config<UNIT>::TYPE == Traits<IP>::MAC)
//         config_by_mac();
// }
// void My_Protocol::init(unsigned int unit)
// {
//     db<Init, My_Protocol>(TRC) << "My_Protocol::init(u=" << unit << ")" << endl;

//     _networks[unit] = new (SYSTEM) My_Protocol(unit);
// }

__END_SYS
#endif
