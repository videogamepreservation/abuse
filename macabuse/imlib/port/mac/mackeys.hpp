#ifndef MACKEYS_HPP
#define MACKEYS_HPP

enum mac_modnum 
{
	MAC_COMMAND = 0x37,
	MAC_LSHIFT,
	MAC_CAPSLOCK,
	MAC_LOPTION,
	MAC_LCONTROL,
	MAC_RSHIFT,
	MAC_ROPTION,
	MAC_RCONTROL,
	MAC_LAST
};

extern int mac_map[8];

#endif
