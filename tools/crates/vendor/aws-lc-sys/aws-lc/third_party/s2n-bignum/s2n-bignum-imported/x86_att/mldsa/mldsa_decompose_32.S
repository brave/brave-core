// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Coefficient decomposition for ML-DSA (GAMMA2 = (Q-1)/32, ML-DSA-65/87)
// Input a0[256] (signed 32-bit words in standard representation, 0 <= c < Q);
// outputs high part a1[256] and low part a0[256] (in place).
//
// For each coefficient c of the input polynomial, computes high and low bits
// c1, c0 such that c = c1*ALPHA + c0 with -ALPHA/2 < c0 <= ALPHA/2, except if
// c1 = (Q-1)/ALPHA where c1 = 0 and -ALPHA/2 <= c0 = c - Q < 0, for
// ALPHA = 2*GAMMA2 = (Q-1)/16.
//
// extern void mldsa_decompose_32(int32_t a1[static 256], int32_t a0[static 256]);
//
// Standard x86-64 ABI: RDI = a1, RSI = a0
// Microsoft x64 ABI:   RCX = a1, RDX = a0
// ----------------------------------------------------------------------------

// This implementation is derived from the public domain AVX2 Dilithium
// implementation (pq-crystals/dilithium) via the mldsa-native project.

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_decompose_32)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_decompose_32)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_decompose_32)
        .text
        .balign 4

// Decompose one 8-lane vector at byte offset \offset. The high part is
// written to a1 (%rdi), the recomputed low part back to a0 (%rsi).
.macro decompose_vector offset
        vmovdqa  \offset(%rsi), %ymm0
        vpaddd   %ymm10, %ymm0, %ymm1
        vpsrld   $7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld  %ymm14, %ymm1, %ymm2
        vpsubd   %ymm2, %ymm0, %ymm2
        vpandn   %ymm1, %ymm3, %ymm1
        vpaddd   %ymm3, %ymm2, %ymm2
        vmovdqa  %ymm1, \offset(%rdi)
        vmovdqa  %ymm2, \offset(%rsi)
.endm

S2N_BN_SYMBOL(mldsa_decompose_32):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_DEC_RSP(96)
        CFI_STACKSAVEU(%xmm10,0)
        CFI_STACKSAVEU(%xmm11,16)
        CFI_STACKSAVEU(%xmm12,32)
        CFI_STACKSAVEU(%xmm13,48)
        CFI_STACKSAVEU(%xmm14,64)
        CFI_STACKSAVE(%rdi,80)
        CFI_STACKSAVE(%rsi,88)
        movq    %rcx, %rdi
        movq    %rdx, %rsi
#endif

// Set up broadcast constants used by the Barrett high-bits sequence.
        movl    $0x7f, %eax
        vmovd   %eax, %xmm10
        vpbroadcastd %xmm10, %ymm10
        movl    $0x401, %eax
        vmovd   %eax, %xmm11
        vpbroadcastd %xmm11, %ymm11
        movl    $0x200, %eax
        vmovd   %eax, %xmm12
        vpbroadcastd %xmm12, %ymm12
        movl    $0x7be100, %eax
        vmovd   %eax, %xmm13
        vpbroadcastd %xmm13, %ymm13
        movl    $0x7fe00, %eax
        vmovd   %eax, %xmm14
        vpbroadcastd %xmm14, %ymm14

// Process all 256 coefficients (32 vectors of 8 int32 each)
        decompose_vector 0
        decompose_vector 32
        decompose_vector 64
        decompose_vector 96
        decompose_vector 128
        decompose_vector 160
        decompose_vector 192
        decompose_vector 224
        decompose_vector 256
        decompose_vector 288
        decompose_vector 320
        decompose_vector 352
        decompose_vector 384
        decompose_vector 416
        decompose_vector 448
        decompose_vector 480
        decompose_vector 512
        decompose_vector 544
        decompose_vector 576
        decompose_vector 608
        decompose_vector 640
        decompose_vector 672
        decompose_vector 704
        decompose_vector 736
        decompose_vector 768
        decompose_vector 800
        decompose_vector 832
        decompose_vector 864
        decompose_vector 896
        decompose_vector 928
        decompose_vector 960
        decompose_vector 992

#if WINDOWS_ABI
        CFI_STACKLOADU(%xmm10,0)
        CFI_STACKLOADU(%xmm11,16)
        CFI_STACKLOADU(%xmm12,32)
        CFI_STACKLOADU(%xmm13,48)
        CFI_STACKLOADU(%xmm14,64)
        CFI_STACKLOAD(%rdi,80)
        CFI_STACKLOAD(%rsi,88)
        CFI_INC_RSP(96)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_decompose_32)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
