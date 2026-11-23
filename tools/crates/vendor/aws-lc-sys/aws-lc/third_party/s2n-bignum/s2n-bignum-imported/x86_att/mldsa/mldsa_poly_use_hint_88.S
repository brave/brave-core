// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Use the hint to correct the high bits of a polynomial for ML-DSA
// (GAMMA2 = (Q-1)/88, ML-DSA-44). Operates in place on a[256].
//
// For each coefficient a of the input polynomial (a standard canonical
// representative, 0 <= a < Q) and hint bit h in {0,1}, compute UseHint(h, a):
// the high part of the decomposition of a, adjusted up or down by one modulo
// 44 when h = 1. The corrected high part (in [0,44)) is written back in place.
//
// extern void mldsa_poly_use_hint_88(int32_t a[static 256],
//                                    const int32_t h[static 256]);
//
// Standard x86-64 ABI: RDI = a, RSI = h
// Microsoft x64 ABI:   RCX = a, RDX = h
// ----------------------------------------------------------------------------

// This implementation is derived from the public domain AVX2 Dilithium
// implementation (pq-crystals/dilithium) via the mldsa-native project.

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_poly_use_hint_88)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_poly_use_hint_88)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_poly_use_hint_88)
        .text
        .balign 4

S2N_BN_SYMBOL(mldsa_poly_use_hint_88):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_DEC_RSP(128)
        CFI_STACKSAVEU(%xmm6,0)
        CFI_STACKSAVEU(%xmm7,16)
        CFI_STACKSAVEU(%xmm8,32)
        CFI_STACKSAVEU(%xmm9,48)
        CFI_STACKSAVEU(%xmm10,64)
        CFI_STACKSAVEU(%xmm11,80)
        CFI_STACKSAVEU(%xmm12,96)
        CFI_STACKSAVE(%rdi,112)
        CFI_STACKSAVE(%rsi,120)
        movq    %rcx, %rdi
        movq    %rdx, %rsi
#endif

// Set up broadcast constants used by the Barrett high-bits sequence.
        movl    $0x7f, %ecx
        xorl    %eax, %eax
        vpxor   %xmm5, %xmm5, %xmm5
        movl    $0x2c0b, %r8d
        vmovd   %r8d, %xmm8
        vpbroadcastd %xmm8, %ymm8
        vmovd   %ecx, %xmm4
        movl    $0x7e6c00, %ecx
        movl    $0x80, %r9d
        vmovd   %r9d, %xmm7
        vpbroadcastd %xmm7, %ymm7
        movl    $0x2b, %r10d
        vmovd   %r10d, %xmm6
        vpbroadcastd %xmm6, %ymm6
        vmovd   %ecx, %xmm3
        vpbroadcastd %xmm4, %ymm4
        vpbroadcastd %xmm3, %ymm3

Lmldsa_poly_use_hint_88_loop:
        vmovdqa   (%rdi), %ymm0
        vmovdqa   (%rsi), %ymm1
        vpaddd    %ymm0, %ymm4, %ymm10
        vpsrld    $0x7, %ymm10, %ymm10
        vpmulhuw  %ymm8, %ymm10, %ymm10
        vpmulhrsw %ymm7, %ymm10, %ymm10
        vpcmpgtd  %ymm3, %ymm0, %ymm11
        vpandn    %ymm10, %ymm11, %ymm9
        vpslld    $0x1, %ymm10, %ymm12
        vpaddd    %ymm10, %ymm12, %ymm12
        vpslld    $0x5, %ymm12, %ymm10
        vpsubd    %ymm12, %ymm10, %ymm10
        vpslld    $0xb, %ymm10, %ymm10
        vpsubd    %ymm10, %ymm0, %ymm0
        vpaddd    %ymm11, %ymm0, %ymm0
        vpcmpgtd  %ymm5, %ymm0, %ymm0
        vpandn    %ymm1, %ymm0, %ymm0
        vpslld    $0x1, %ymm0, %ymm0
        vpsubd    %ymm0, %ymm1, %ymm0
        vpaddd    %ymm9, %ymm0, %ymm0
        vpblendvb %ymm0, %ymm6, %ymm0, %ymm0
        vpcmpgtd  %ymm6, %ymm0, %ymm1
        vpandn    %ymm0, %ymm1, %ymm0
        vmovdqa   %ymm0, (%rdi)
        addq      $0x20, %rdi
        addq      $0x20, %rsi
        addq      $0x20, %rax
        cmpq      $0x400, %rax
        jne       Lmldsa_poly_use_hint_88_loop

#if WINDOWS_ABI
        CFI_STACKLOADU(%xmm6,0)
        CFI_STACKLOADU(%xmm7,16)
        CFI_STACKLOADU(%xmm8,32)
        CFI_STACKLOADU(%xmm9,48)
        CFI_STACKLOADU(%xmm10,64)
        CFI_STACKLOADU(%xmm11,80)
        CFI_STACKLOADU(%xmm12,96)
        CFI_STACKLOAD(%rdi,112)
        CFI_STACKLOAD(%rsi,120)
        CFI_INC_RSP(128)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_poly_use_hint_88)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
