// x2: key schedule
// x1: input
// x0: output
#include "_internal_s2n_bignum.h"

        S2N_BN_SYM_VISIBILITY_DIRECTIVE(aes_decrypt)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(aes_decrypt)
        .text
        .balign 4
        .arch armv8-a+crypto

aes_decrypt:
	ldr	w6,[x2,#240]
	ld1	{v0.16b},[x2],#16
	ld1	{v1.16b},[x2],#16
	ld1	{v6.16b},[x1]
	sub	w6,w6,#2

Loop_dec:
	aesd	v6.16b,v0.16b
	aesimc	v6.16b,v6.16b
	ld1	{v0.4s},[x2],#16
	subs	w6,w6,#2
	aesd	v6.16b,v1.16b
	aesimc	v6.16b,v6.16b
	ld1	{v1.4s},[x2],#16
	b.gt	Loop_dec

	aesd	v6.16b,v0.16b
	aesimc	v6.16b,v6.16b
	ld1	{v0.4s},[x2]
	aesd	v6.16b,v1.16b
	eor	v6.16b,v6.16b,v0.16b
	st1 {v6.4s},[x0]
