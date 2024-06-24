// SPDX-License-Identifier: MIT
// Copyright © 2018-2021 WireGuard LLC. All Rights Reserved.

#include "key.h"
#include "x25519.h"


//
// Note from CJ 2024-06-13
// Xcode 16 beta 1 broke compatibility by removing the type definitions for u_int32_t
// which are old lingering references to the BSD origins of xnu
// https://opensource.apple.com/source/xnu/xnu-344/bsd/sys/kern_control.h.auto.html
// There are two ways to resolve this
//
// this way
//typedef unsigned int u_int32_t;
//typedef unsigned char u_char;
//typedef unsigned short u_int16_t;
//
// or
// typedef uint32_t u_int32_t;
//typedef uint8_t u_char;
//typedef uint16_t u_int16_t;
//
// I opted for skipping right past all of the typedefs and using the solution below

/* From <sys/kern_control.h> */
#define CTLIOCGINFO 0xc0644e03UL
struct ctl_info {
	unsigned int   ctl_id;
    char        ctl_name[96];
};
struct sockaddr_ctl {
    unsigned char      sc_len;
    unsigned char      sc_family;
    unsigned short   ss_sysaddr;
    unsigned int   sc_id;
	unsigned int   sc_unit;
	unsigned int   sc_reserved[5];
};
