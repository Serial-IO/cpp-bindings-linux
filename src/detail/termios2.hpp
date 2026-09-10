#pragma once

#include <termios.h>

#ifndef TCGETS2
#define TCGETS2 0x802C542A
#define TCSETS2 0x402C542B
#endif

#ifndef BOTHER
// Linux asm-generic/termbits.h: select the explicit c_ispeed/c_ospeed values.
#define BOTHER 0x00001000
#endif

#ifndef CRTSCTS
#define CRTSCTS 020000000000
#endif

// NOLINTBEGIN
struct termios2
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[19];
    speed_t c_ispeed;
    speed_t c_ospeed;
};
// NOLINTEND
