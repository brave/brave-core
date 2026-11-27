// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT-0

// ----------------------------------------------------------------------------
// Reduce modulo group order, z := x mod n_256k1
// Input x[k]; output z[4]
//
//    extern void bignum_mod_n256k1(uint64_t z[static 4], uint64_t k,
//                                  const uint64_t *x);
//
// Reduction is modulo the group order of the secp256k1 curve.
//
// Standard x86-64 ABI: RDI = z, RSI = k, RDX = x
// Microsoft x64 ABI:   RCX = z, RDX = k, R8 = x
// ----------------------------------------------------------------------------

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(bignum_mod_n256k1)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(bignum_mod_n256k1)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(bignum_mod_n256k1)
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

#define q %rbx
#define t %r13

S2N_BN_SYMBOL(bignum_mod_n256k1):
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
        CFI_PUSH(%r13)

// If the input is already <= 3 words long, go to a trivial "copy" path

        cmpq    $4, k
        jc      Lbignum_mod_n256k1_shortinput

// Otherwise load the top 4 digits (top-down) and reduce k by 4

        subq    $4, k
        movq    24(%rdx,k,8), m3
        movq    16(%rdx,k,8), m2
        movq    8(%rdx,k,8), m1
        movq    (%rdx,k,8), m0

// Move x into another register to leave %rdx free for multiplies

        movq    %rdx, x

// Reduce the top 4 digits mod n_256k1 (conditional subtraction of n_256k1).
// We add 2^256 - n_256k1 = [0; 1; 0x4551231950b75fc4; 0x402da1732fc9bebf];
// if this carries, value >= n_256k1 so keep the reduced value.

        movq    $0x402da1732fc9bebf, %rax
        movq    $0x4551231950b75fc4, q

        addq    %rax, m0
        adcq    q, m1
        adcq    $1, m2
        adcq    $0, m3
        sbbq    d, d
        notq    d
        andq    d, %rax
        andq    d, q
        andq    $1, d
        subq    %rax, m0
        sbbq    q, m1
        sbbq    d, m2
        sbbq    $0, m3

// Now do (k-4) iterations of 5->4 word modular reduction

        testq   k, k
        jz      Lbignum_mod_n256k1_writeback

Lbignum_mod_n256k1_loop:

// Writing the input as z = 2^256 * m3 + rest, our quotient approximation is
// q = min(m3 + 1, 2^64 - 1). This is accurate because 2^256 - n_256k1 is
// only about 2^129, so the true quotient is m3 or m3 + 1.

        movq    m3, %rax
        addq    $1, %rax
        sbbq    %rdx, %rdx
        orq     %rdx, %rax
        movq    %rax, q

// Load the next digit so current m to reduce = [m3;m2;m1;m0;d]

        movq    -8(x,k,8), d

// Compensate m3 for 2^256 * q

        subq    q, m3

// Compute [t3;t2;t1;t0] = q * (2^256 - n_256k1) = q * [0; 1; n1; n0]
// First multiply: q * n0

        movq    $0x402da1732fc9bebf, %rax
        mulq    q
        movq    %rdx, t
        addq    %rax, d
        adcq    $0, t

// Second multiply: q * n1

        movq    $0x4551231950b75fc4, %rax
        mulq    q
        addq    %rax, t
        adcq    $0, %rdx
        addq    q, %rdx
        movl    $0, %eax
        adcq    $0, %rax

// Add product to residue [m3;m2;m1;m0] (t0 already added to d)

        addq    t, m0
        adcq    %rdx, m1
        adcq    %rax, m2
        adcq    $0, m3

// Now our top word m3 is either zero or all 1s. Use it for a masked
// addition of n_256k1, which we can do by a *subtraction* of
// 2^256 - n_256k1 from our portion. Compute all masks first since
// AND clears CF, which would break the sub/sbb borrow chain.

        movq    $0x402da1732fc9bebf, %rax
        andq    m3, %rax
        movq    $0x4551231950b75fc4, %rdx
        andq    m3, %rdx
        movq    $1, t
        andq    m3, t

        subq    %rax, d
        sbbq    %rdx, m0
        sbbq    t, m1
        sbbq    $0, m2

// Shuffle registers up and loop

        movq    m2, m3
        movq    m1, m2
        movq    m0, m1
        movq    d, m0

        decq    k
        jnz     Lbignum_mod_n256k1_loop

// Write back

Lbignum_mod_n256k1_writeback:

        movq    m0, (z)
        movq    m1, 8(z)
        movq    m2, 16(z)
        movq    m3, 24(z)

// Restore registers and return

        CFI_POP(%r13)
        CFI_POP(%r12)
        CFI_POP(%rbx)
#if WINDOWS_ABI
        CFI_POP(%rsi)
        CFI_POP(%rdi)
#endif
        CFI_RET

Lbignum_mod_n256k1_shortinput:

        xorq    m0, m0
        xorq    m1, m1
        xorq    m2, m2
        xorq    m3, m3

        testq   k, k
        jz      Lbignum_mod_n256k1_writeback
        movq    (%rdx), m0
        decq    k
        jz      Lbignum_mod_n256k1_writeback
        movq    8(%rdx), m1
        decq    k
        jz      Lbignum_mod_n256k1_writeback
        movq    16(%rdx), m2
        jmp     Lbignum_mod_n256k1_writeback

S2N_BN_SIZE_DIRECTIVE(bignum_mod_n256k1)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
