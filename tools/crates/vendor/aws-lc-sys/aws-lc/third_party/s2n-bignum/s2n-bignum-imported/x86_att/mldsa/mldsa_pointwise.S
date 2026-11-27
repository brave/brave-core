// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Pointwise multiplication of polynomials in NTT domain for ML-DSA
// Inputs c[256], a[256], b[256], qdata (all signed 32-bit words); output c[256]
//
// Performs pointwise multiplication of two polynomials in NTT domain with
// Montgomery reduction. Given two polynomials a and b in NTT representation
// (256 coefficients each), computes c = a * b (coefficient-wise) in NTT domain.
// Each coefficient multiplication is followed by Montgomery reduction modulo
// Q = 8380417.
//
// The input polynomials a and b are assumed to have coefficients bounded by
// 9*Q in absolute value. The output polynomial c will have coefficients bounded
// by Q in absolute value after Montgomery reduction.
//
// The qdata parameter points to precomputed constants required for Montgomery
// arithmetic:
//   - 8xQINV: Eight copies of Q^(-1) mod 2^32 for Montgomery multiplication
//   - 8xQ: Eight copies of Q for the reduction step
//
// extern void mldsa_pointwise_x86(
//     int32_t c[static 256],
//     const int32_t a[static 256],
//     const int32_t b[static 256],
//     const int32_t qdata[static 16]
// );
//
// Standard x86-64 ABI: RDI = c, RSI = a, RDX = b, RCX = qdata
// Microsoft x64 ABI:   RCX = c, RDX = a, R8 = b, R9 = qdata
// ----------------------------------------------------------------------------

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_pointwise_x86)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_pointwise_x86)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_pointwise_x86)
        .text
        .balign 32

#define c %rdi
#define a %rsi
#define b %rdx
#define qdata %rcx

#define MLD_AVX2_BACKEND_DATA_OFFSET_8XQINV 0
#define MLD_AVX2_BACKEND_DATA_OFFSET_8XQ 8

S2N_BN_SYMBOL(mldsa_pointwise_x86):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_DEC_RSP(192)
        CFI_STACKSAVEU(%xmm6,0)
        CFI_STACKSAVEU(%xmm7,16)
        CFI_STACKSAVEU(%xmm8,32)
        CFI_STACKSAVEU(%xmm9,48)
        CFI_STACKSAVEU(%xmm10,64)
        CFI_STACKSAVEU(%xmm11,80)
        CFI_STACKSAVEU(%xmm12,96)
        CFI_STACKSAVEU(%xmm13,112)
        CFI_STACKSAVEU(%xmm14,128)
        CFI_STACKSAVEU(%xmm15,144)
        CFI_STACKSAVE(%rdi,160)
        CFI_STACKSAVE(%rsi,168)
        CFI_STACKSAVE(%rdx,176)
        movq    %rcx, %rdi
        movq    %rdx, %rsi
        movq    %r8, %rdx
        movq    %r9, %rcx
#endif

// Load constants from qdata
        vmovdqa (MLD_AVX2_BACKEND_DATA_OFFSET_8XQINV)*4(qdata), %ymm0
        vmovdqa (MLD_AVX2_BACKEND_DATA_OFFSET_8XQ)*4(qdata), %ymm1

        xorl    %eax, %eax
Lpointwise_looptop1:
// Handle 24 = 3*8 coefficients per iteration

// Load a's coefficients
        vmovdqa (a), %ymm2
        vmovdqa 32(a), %ymm4
        vmovdqa 64(a), %ymm6
// Load b's coefficients
        vmovdqa (b), %ymm10
        vmovdqa 32(b), %ymm12
        vmovdqa 64(b), %ymm14

// Extract high 32-bit parts for Montgomery multiplication
        vpsrlq  $32, %ymm2, %ymm3
        vpsrlq  $32, %ymm4, %ymm5
        vmovshdup %ymm6, %ymm7
        vpsrlq  $32, %ymm10, %ymm11
        vpsrlq  $32, %ymm12, %ymm13
        vmovshdup %ymm14, %ymm15
        /*
         * %ymm{i} stores a's coefficients for i in 2...7, and b's coefficients
         * for i in 10...15.
         *
         * Bounds: |ymm{i}| < 9q for i in 2...7, 10...15
         */

// Multiply coefficients
        vpmuldq %ymm10, %ymm2, %ymm2
        vpmuldq %ymm11, %ymm3, %ymm3
        vpmuldq %ymm12, %ymm4, %ymm4
        vpmuldq %ymm13, %ymm5, %ymm5
        vpmuldq %ymm14, %ymm6, %ymm6
        vpmuldq %ymm15, %ymm7, %ymm7
        /*
         * Bounds: |ymm{i}| < 81q^2 < MONTGOMERY_REDUCE_STRONG_DOMAIN_MAX
         *             for i in 2...7
         */

// Montgomery reduction
        vpmuldq %ymm2, %ymm0, %ymm10
        vpmuldq %ymm3, %ymm0, %ymm11
        vpmuldq %ymm4, %ymm0, %ymm12
        vpmuldq %ymm5, %ymm0, %ymm13
        vpmuldq %ymm6, %ymm0, %ymm14
        vpmuldq %ymm7, %ymm0, %ymm15
        vpmuldq %ymm10, %ymm1, %ymm10
        vpmuldq %ymm11, %ymm1, %ymm11
        vpmuldq %ymm12, %ymm1, %ymm12
        vpmuldq %ymm13, %ymm1, %ymm13
        vpmuldq %ymm14, %ymm1, %ymm14
        vpmuldq %ymm15, %ymm1, %ymm15
        vpsubq  %ymm10, %ymm2, %ymm2
        vpsubq  %ymm11, %ymm3, %ymm3
        vpsubq  %ymm12, %ymm4, %ymm4
        vpsubq  %ymm13, %ymm5, %ymm5
        vpsubq  %ymm14, %ymm6, %ymm6
        vpsubq  %ymm15, %ymm7, %ymm7
        /*
         * All coefficients are Montgomery-reduced, resulting in
         *
         * Bounds: |ymm{i}| < q for i in 2...7
         *
         * See description of mld_montgomery_reduce() in mldsa/src/reduce.h.
         */

// Repack and store results
        vpsrlq  $32, %ymm2, %ymm2
        vpsrlq  $32, %ymm4, %ymm4
        vmovshdup %ymm6, %ymm6
        vpblendd $0xAA, %ymm3, %ymm2, %ymm2
        vpblendd $0xAA, %ymm5, %ymm4, %ymm4
        vpblendd $0xAA, %ymm7, %ymm6, %ymm6
        vmovdqa %ymm2, (c)
        vmovdqa %ymm4, 32(c)
        vmovdqa %ymm6, 64(c)

        addq    $96, c
        addq    $96, a
        addq    $96, b
        addl    $1, %eax
        cmpl    $10, %eax
        jb      Lpointwise_looptop1


// Handle the last 256 % 24 = 16 = 2*8 coefficients, left over by the loop

// Load remaining a's coefficients
        vmovdqa (a), %ymm2
        vmovdqa 32(a), %ymm4
// Load remaining b's coefficients
        vmovdqa (b), %ymm10
        vmovdqa 32(b), %ymm12

// Extract high 32-bit parts
        vpsrlq  $32, %ymm2, %ymm3
        vpsrlq  $32, %ymm4, %ymm5
        vmovshdup %ymm10, %ymm11
        vmovshdup %ymm12, %ymm13
        /*
         * %ymm{i} stores a's coefficients for i in 2...5, and b's coefficients
         * for i in 10...13.
         *
         * Bounds: |ymm{i}| < 9q for i in 2...5, 10...13
         */

// Multiply remaining coefficients
        vpmuldq %ymm10, %ymm2, %ymm2
        vpmuldq %ymm11, %ymm3, %ymm3
        vpmuldq %ymm12, %ymm4, %ymm4
        vpmuldq %ymm13, %ymm5, %ymm5
        /*
         * Bounds: |ymm{i}| < 81q^2 < MONTGOMERY_REDUCE_STRONG_DOMAIN_MAX
         *             for i in 2...5
         */

// Montgomery reduction for remaining coefficients
        vpmuldq %ymm2, %ymm0, %ymm10
        vpmuldq %ymm3, %ymm0, %ymm11
        vpmuldq %ymm4, %ymm0, %ymm12
        vpmuldq %ymm5, %ymm0, %ymm13
        vpmuldq %ymm10, %ymm1, %ymm10
        vpmuldq %ymm11, %ymm1, %ymm11
        vpmuldq %ymm12, %ymm1, %ymm12
        vpmuldq %ymm13, %ymm1, %ymm13
        vpsubq  %ymm10, %ymm2, %ymm2
        vpsubq  %ymm11, %ymm3, %ymm3
        vpsubq  %ymm12, %ymm4, %ymm4
        vpsubq  %ymm13, %ymm5, %ymm5
        /*
         * As explained in the loop.
         *
         * Bounds: |ymm{i}| < q for i in 2...5
         */

// Repack and store final results
        vpsrlq  $32, %ymm2, %ymm2
        vmovshdup %ymm4, %ymm4
        vpblendd $0x55, %ymm2, %ymm3, %ymm2
        vpblendd $0x55, %ymm4, %ymm5, %ymm4
        vmovdqa %ymm2, (c)
        vmovdqa %ymm4, 32(c)

#if WINDOWS_ABI
        CFI_STACKLOADU(%xmm6,0)
        CFI_STACKLOADU(%xmm7,16)
        CFI_STACKLOADU(%xmm8,32)
        CFI_STACKLOADU(%xmm9,48)
        CFI_STACKLOADU(%xmm10,64)
        CFI_STACKLOADU(%xmm11,80)
        CFI_STACKLOADU(%xmm12,96)
        CFI_STACKLOADU(%xmm13,112)
        CFI_STACKLOADU(%xmm14,128)
        CFI_STACKLOADU(%xmm15,144)
        CFI_STACKLOAD(%rdi,160)
        CFI_STACKLOAD(%rsi,168)
        CFI_STACKLOAD(%rdx,176)
        CFI_INC_RSP(192)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_pointwise_x86)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
