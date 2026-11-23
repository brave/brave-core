// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Infinity-norm bound check on polynomial coefficients for ML-DSA
// Input a[256] (signed 32-bit words), scalar bound
//
// Checks the infinity norm of the polynomial against the given bound.
// Returns 0 if every |coefficient| is strictly smaller than the bound;
// otherwise returns 1 (i.e. returns 1 if any |coefficient| >= bound).
// The bound is assumed to be non-negative.
//
// extern uint64_t mldsa_chknorm(const int32_t a[static 256], uint64_t bound);
//
// Standard x86-64 ABI: RDI = a, RSI = bound, returns RAX
// Microsoft x64 ABI:   RCX = a, RDX = bound, returns RAX
// ----------------------------------------------------------------------------

// This implementation is derived from the public domain AVX2 Dilithium
// implementation (pq-crystals/dilithium) via the mldsa-native project.

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_chknorm)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_chknorm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_chknorm)
        .text

// For each 8-lane vector: OR into the accumulator %ymm1 a mask that is
// all-ones in any lane where |coefficient| > (bound - 1), i.e. >= bound.
.macro chknorm_vector offset, reg
        vpabsd   \offset(%rdi), \reg
        vpcmpgtd %ymm2, \reg, \reg
        vpor    \reg, %ymm1, %ymm1
.endm

S2N_BN_SYMBOL(mldsa_chknorm):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_PUSH(%rdi)
        CFI_PUSH(%rsi)
        movq    %rcx, %rdi
        movq    %rdx, %rsi
#endif

// Set up constants: %ymm1 = zero accumulator, %ymm2 = (bound - 1) broadcast
        subl    $1, %esi
        vpxor   %xmm1, %xmm1, %xmm1
        vmovd   %esi, %xmm2
        vpbroadcastd %xmm2, %ymm2

// Process all 256 coefficients (32 vectors of 8 int32 each)
        chknorm_vector 0, %ymm0
        chknorm_vector 32, %ymm3
        chknorm_vector 64, %ymm4
        chknorm_vector 96, %ymm5
        chknorm_vector 128, %ymm0
        chknorm_vector 160, %ymm3
        chknorm_vector 192, %ymm4
        chknorm_vector 224, %ymm5
        chknorm_vector 256, %ymm0
        chknorm_vector 288, %ymm3
        chknorm_vector 320, %ymm4
        chknorm_vector 352, %ymm5
        chknorm_vector 384, %ymm0
        chknorm_vector 416, %ymm3
        chknorm_vector 448, %ymm4
        chknorm_vector 480, %ymm5
        chknorm_vector 512, %ymm0
        chknorm_vector 544, %ymm3
        chknorm_vector 576, %ymm4
        chknorm_vector 608, %ymm5
        chknorm_vector 640, %ymm0
        chknorm_vector 672, %ymm3
        chknorm_vector 704, %ymm4
        chknorm_vector 736, %ymm5
        chknorm_vector 768, %ymm0
        chknorm_vector 800, %ymm3
        chknorm_vector 832, %ymm4
        chknorm_vector 864, %ymm5
        chknorm_vector 896, %ymm0
        chknorm_vector 928, %ymm3
        chknorm_vector 960, %ymm4
        chknorm_vector 992, %ymm5

// Return 1 iff the accumulator has any nonzero lane, else 0
        xorl    %eax, %eax
        vptest  %ymm1, %ymm1
        setne   %al

#if WINDOWS_ABI
        CFI_POP(%rsi)
        CFI_POP(%rdi)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_chknorm)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
