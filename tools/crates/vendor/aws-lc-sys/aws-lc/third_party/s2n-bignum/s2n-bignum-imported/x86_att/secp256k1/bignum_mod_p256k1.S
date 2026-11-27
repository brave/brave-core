// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT-0

// ----------------------------------------------------------------------------
// Reduce modulo field characteristic, z := x mod p_256k1
// Input x[k]; output z[4]
//
//    extern void bignum_mod_p256k1(uint64_t z[static 4], uint64_t k,
//                                  const uint64_t *x);
//
// Standard x86-64 ABI: RDI = z, RSI = k, RDX = x
// Microsoft x64 ABI:   RCX = z, RDX = k, R8 = x
// ----------------------------------------------------------------------------

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(bignum_mod_p256k1)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(bignum_mod_p256k1)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(bignum_mod_p256k1)
        .text
        .balign 32

#define z %rdi
#define k %rsi
#define x %rcx

#define m0 %r8
#define m1 %r9
#define m2 %r10
#define m3 %r11
#define d %r12

#define c %rbx

S2N_BN_SYMBOL(bignum_mod_p256k1):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_PUSH(%rdi)
        CFI_PUSH(%rsi)
        movq    %rcx, %rdi
        movq    %rdx, %rsi
        movq    %r8, %rdx
#endif

// Save extra registers

        CFI_PUSH(%rbx)
        CFI_PUSH(%r12)

// If the input is already <= 3 words long, go to a trivial "copy" path

        cmpq    $4, k
        jc      Lbignum_mod_p256k1_shortinput

// Otherwise load the top 4 digits (top-down) and reduce k by 4

        subq    $4, k
        movq    24(%rdx,k,8), m3
        movq    16(%rdx,k,8), m2
        movq    8(%rdx,k,8), m1
        movq    (%rdx,k,8), m0

// Move x into another register to leave %rdx free for multiplies

        movq    %rdx, x

// Load the constant c = 4294968273, i.e. p_256k1 = 2^256 - c

        movq    $0x1000003D1, c

// Reduce the top 4 digits mod p_256k1 (conditional subtraction of p_256k1).
// We add c = 2^256 - p_256k1; if this carries, value >= p_256k1 so keep.

        addq    c, m0
        adcq    $0, m1
        adcq    $0, m2
        adcq    $0, m3

// If no carry (CF=0), the value was < p_256k1, so undo the addition

        sbbq    %rax, %rax
        notq    %rax
        andq    c, %rax
        subq    %rax, m0
        sbbq    $0, m1
        sbbq    $0, m2
        sbbq    $0, m3

// Now do (k-4) iterations of 5->4 word modular reduction

        testq   k, k
        jz      Lbignum_mod_p256k1_writeback

Lbignum_mod_p256k1_loop:

// Writing the input as z = 2^256 * m3 + rest, our quotient approximation is
// q = min(m3 + 1, 2^64 - 1). This is accurate because p_256k1 = 2^256 - c
// with c only 33 bits, so the true quotient is m3 or m3 + 1.

        movq    m3, %rax
        addq    $1, %rax
        sbbq    %rdx, %rdx
        orq     %rdx, %rax

// Load the next digit so current m to reduce = [m3;m2;m1;m0;d]

        movq    -8(x,k,8), d

// Now form [m3;m2;m1;m0;d] = m - q * p_256k1
// = m - q * (2^256 - c) = m - q*2^256 + q*c
// First compensate for the 2^256 * q term

        subq    %rax, m3

// Now [%rdx;%rax] = q * c (mul uses %rax as implicit source, q is still in %rax)

        mulq    c

// [m3;m2;m1;m0;d] += [0;0;0;%rdx;%rax]

        addq    %rax, d
        adcq    %rdx, m0
        adcq    $0, m1
        adcq    $0, m2
        adcq    $0, m3

// Now our top word m3 is either zero or all 1s. Use it for a masked
// subtraction of c (equivalently a masked addition of p_256k1)

        movq    c, %rax
        andq    m3, %rax
        subq    %rax, d
        sbbq    $0, m0
        sbbq    $0, m1
        sbbq    $0, m2

// Shuffle registers up and loop

        movq    m2, m3
        movq    m1, m2
        movq    m0, m1
        movq    d, m0

        decq    k
        jnz     Lbignum_mod_p256k1_loop

// Write back

Lbignum_mod_p256k1_writeback:

        movq    m0, (z)
        movq    m1, 8(z)
        movq    m2, 16(z)
        movq    m3, 24(z)

// Restore registers and return

        CFI_POP(%r12)
        CFI_POP(%rbx)
#if WINDOWS_ABI
        CFI_POP(%rsi)
        CFI_POP(%rdi)
#endif
        CFI_RET

Lbignum_mod_p256k1_shortinput:

        xorq    m0, m0
        xorq    m1, m1
        xorq    m2, m2
        xorq    m3, m3

        testq   k, k
        jz      Lbignum_mod_p256k1_writeback
        movq    (%rdx), m0
        decq    k
        jz      Lbignum_mod_p256k1_writeback
        movq    8(%rdx), m1
        decq    k
        jz      Lbignum_mod_p256k1_writeback
        movq    16(%rdx), m2
        jmp     Lbignum_mod_p256k1_writeback

S2N_BN_SIZE_DIRECTIVE(bignum_mod_p256k1)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
