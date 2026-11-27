// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT-0

// ----------------------------------------------------------------------------
// Reduce modulo field characteristic, z := x mod p_256k1
// Input x[k]; output z[4]
//
//    extern void bignum_mod_p256k1(uint64_t z[static 4], uint64_t k,
//                                  const uint64_t *x);
//
// Standard ARM ABI: X0 = z, X1 = k, X2 = x
// ----------------------------------------------------------------------------

#include "_internal_s2n_bignum_arm.h"

        S2N_BN_SYM_VISIBILITY_DIRECTIVE(bignum_mod_p256k1)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(bignum_mod_p256k1)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(bignum_mod_p256k1)
        .text
        .balign 4

#define z x0
#define k x1
#define x x2

#define m0 x3
#define m1 x4
#define m2 x5
#define m3 x6

#define t0 x7
#define t1 x8
#define t2 x9
#define t3 x10
#define t4 x11

#define c x12

// These two are aliased: we only load d when finished with q

#define q x13
#define d x13


S2N_BN_SYMBOL(bignum_mod_p256k1):
        CFI_START

// If the input is already <= 3 words long, go to a trivial "copy" path

        cmp     k, #4
        bcc     Lbignum_mod_p256k1_short

// Otherwise load the top 4 digits (top-down) and reduce k by 4

        sub     k, k, #4
        lsl     t0, k, #3
        add     t0, t0, x
        ldp     m2, m3, [t0, #16]
        ldp     m0, m1, [t0]

// Load the constant c = 4294968273, i.e. p_256k1 = 2^256 - c

        mov     c, #977
        orr     c, c, #0x100000000

// Reduce the top 4 digits mod p_256k1 (a conditional subtraction of p_256k1)

        adds    t0, m0, c
        adcs    t1, m1, xzr
        adcs    t2, m2, xzr
        adcs    t3, m3, xzr

        csel    m0, m0, t0, cc
        csel    m1, m1, t1, cc
        csel    m2, m2, t2, cc
        csel    m3, m3, t3, cc

// Now do (k-4) iterations of 5->4 word modular reduction

        cbz     k, Lbignum_mod_p256k1_writeback
Lbignum_mod_p256k1_loop:

// Writing the input as z = 2^256 * m3 + rest, our quotient approximation is
// q = min(m3 + 1, 2^64 - 1). This is accurate because p_256k1 = 2^256 - c
// with c only 33 bits, so the true quotient is m3 or m3 + 1.

        adds    q, m3, #1
        csinv   q, q, xzr, cc

// Now the quotient estimate is q, and then we do the reduction, writing
// z = [m3;m2;m1;m0], as z' = (2^256 * m3 + z) - q * p_256k1 =
// (2^256 * (m3 - q) + z) + q * c

        mul     t0, q, c
        umulh   t1, q, c

// Compensate for 2^256 * q

        sub     m3, m3, q

// Decrement k and load the next digit (note d aliases q)

        sub     k, k, #1
        ldr     d, [x, k, lsl #3]

// [t4;t3;t2;t1;t0] = [m3;m2;m1;m0;d] - q * p_256k1

        adds    t0, d, t0
        adcs    t1, m0, t1
        adcs    t2, m1, xzr
        adcs    t3, m2, xzr
        adc     t4, m3, xzr

// Now our top word t4 is either zero or all 1s. Use it for a masked
// addition of p_256k1, which we can do by a *subtraction* of c.

        and     d, t4, c
        subs    m0, t0, d
        sbcs    m1, t1, xzr
        sbcs    m2, t2, xzr
        sbc     m3, t3, xzr

        cbnz    k, Lbignum_mod_p256k1_loop

// Finally write back [m3;m2;m1;m0] and return

Lbignum_mod_p256k1_writeback:
        stp     m0, m1, [z]
        stp     m2, m3, [z, #16]
        CFI_RET

// Short case: just copy the input with zero-padding

Lbignum_mod_p256k1_short:
        mov     m0, xzr
        mov     m1, xzr
        mov     m2, xzr
        mov     m3, xzr

        cbz     k, Lbignum_mod_p256k1_writeback
        ldr     m0, [x]
        subs    k, k, #1
        beq     Lbignum_mod_p256k1_writeback
        ldr     m1, [x, #8]
        subs    k, k, #1
        beq     Lbignum_mod_p256k1_writeback
        ldr     m2, [x, #16]
        b       Lbignum_mod_p256k1_writeback

S2N_BN_SIZE_DIRECTIVE(bignum_mod_p256k1)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
