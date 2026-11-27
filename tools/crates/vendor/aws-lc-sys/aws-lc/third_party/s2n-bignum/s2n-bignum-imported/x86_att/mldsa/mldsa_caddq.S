// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Conditional addition of Q to polynomial coefficients for ML-DSA
// Input/output a[256] (signed 32-bit words)
//
// For each coefficient x in (-Q, Q), computes x rem Q in [0, Q).
// If x < 0, adds Q = 8380417; otherwise leaves x unchanged.
//
// extern void mldsa_caddq(int32_t a[static 256]);
//
// Standard x86-64 ABI: RDI = a
// Microsoft x64 ABI:   RCX = a
// ----------------------------------------------------------------------------

// This implementation is derived from the public domain AVX2 Dilithium
// implementation (pq-crystals/dilithium) via the mldsa-native project.

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_caddq)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_caddq)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_caddq)
        .text

.macro caddq_vector offset, reg
        vpcmpgtd \offset(%rdi), %ymm2, \reg
        vpand   %ymm1, \reg, \reg
        vpaddd  \offset(%rdi), \reg, \reg
        vmovdqa  \reg, \offset(%rdi)
.endm

S2N_BN_SYMBOL(mldsa_caddq):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_PUSH(%rdi)
        CFI_PUSH(%rsi)
        movq    %rcx, %rdi
#endif

// Set up constants: %ymm2 = zero, %ymm1 = Q broadcast
        vpxor   %xmm2, %xmm2, %xmm2
        movl    $8380417, %eax
        vmovd   %eax, %xmm1
        vpbroadcastd %xmm1, %ymm1

// Process all 256 coefficients (32 vectors of 8 int32 each)
        caddq_vector 0, %ymm0
        caddq_vector 32, %ymm3
        caddq_vector 64, %ymm4
        caddq_vector 96, %ymm5
        caddq_vector 128, %ymm0
        caddq_vector 160, %ymm3
        caddq_vector 192, %ymm4
        caddq_vector 224, %ymm5
        caddq_vector 256, %ymm0
        caddq_vector 288, %ymm3
        caddq_vector 320, %ymm4
        caddq_vector 352, %ymm5
        caddq_vector 384, %ymm0
        caddq_vector 416, %ymm3
        caddq_vector 448, %ymm4
        caddq_vector 480, %ymm5
        caddq_vector 512, %ymm0
        caddq_vector 544, %ymm3
        caddq_vector 576, %ymm4
        caddq_vector 608, %ymm5
        caddq_vector 640, %ymm0
        caddq_vector 672, %ymm3
        caddq_vector 704, %ymm4
        caddq_vector 736, %ymm5
        caddq_vector 768, %ymm0
        caddq_vector 800, %ymm3
        caddq_vector 832, %ymm4
        caddq_vector 864, %ymm5
        caddq_vector 896, %ymm0
        caddq_vector 928, %ymm3
        caddq_vector 960, %ymm4
        caddq_vector 992, %ymm5

#if WINDOWS_ABI
        CFI_POP(%rsi)
        CFI_POP(%rdi)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_caddq)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
