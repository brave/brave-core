// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Pointwise multiplication with accumulation for ML-DSA L7
// Inputs c[256], a[7*256], b[7*256], qdata (all signed 32-bit words); output c[256]
//
// Performs pointwise multiplication with accumulation across seven polynomial
// vectors in NTT domain with Montgomery reduction. Given seven pairs of
// polynomials (a0,b0), (a1,b1), (a2,b2), (a3,b3), (a4,b4), (a5,b5), (a6,b6)
// each with 256 coefficients in NTT representation, computes
// c = a0*b0 + a1*b1 + a2*b2 + a3*b3 + a4*b4 + a5*b5 + a6*b6
// (coefficient-wise) in NTT domain. Each coefficient multiplication and
// accumulation is followed by Montgomery reduction modulo Q = 8380417.
//
// This function is specifically optimized for ML-DSA Level 7 parameter set
// (ML-DSA-87) which requires accumulating 7 polynomial products (l=7).
// The input polynomials a and b are laid out sequentially with a stride
// of 1024 bytes (256 * 4).
//
// Input bounds (per coefficient, absolute value):
//   |a[i]| < Q
//   |b[i]| < 9*Q
// The output polynomial c will have coefficients bounded by Q in absolute
// value after Montgomery reduction.
//
// The qdata parameter points to precomputed constants required for Montgomery
// arithmetic:
//   - 8xQINV: Eight copies of Q^(-1) mod 2^32 for Montgomery multiplication
//   - 8xQ: Eight copies of Q for the reduction step
//
// extern void mldsa_pointwise_acc_l7_x86(
//     int32_t c[static 256],
//     const int32_t a[static 1792],
//     const int32_t b[static 1792],
//     const int32_t qdata[static 16]
// );
//
// Standard x86-64 ABI: RDI = c, RSI = a, RDX = b, RCX = qdata
// Microsoft x64 ABI:   RCX = c, RDX = a, R8 = b, R9 = qdata
// ----------------------------------------------------------------------------

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_pointwise_acc_l7_x86)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_pointwise_acc_l7_x86)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_pointwise_acc_l7_x86)
        .text
        .balign 32

#define c %rdi
#define a %rsi
#define b %rdx
#define qdata %rcx

#define MLD_AVX2_BACKEND_DATA_OFFSET_8XQ 0
#define MLD_AVX2_BACKEND_DATA_OFFSET_8XQINV 8

.macro pointwise off
// Load a's coefficients
        vmovdqa \off(a), %ymm6
        vmovdqa \off + 32(a), %ymm8
// Load b's coefficients
        vmovdqa \off(b), %ymm10
        vmovdqa \off + 32(b), %ymm12
// Extract high 32-bit parts for Montgomery multiplication
        vpsrlq  $32, %ymm6, %ymm7
        vpsrlq  $32, %ymm8, %ymm9
        vmovshdup %ymm10, %ymm11
        vmovshdup %ymm12, %ymm13
// Multiply coefficients
        vpmuldq %ymm10, %ymm6, %ymm6
        vpmuldq %ymm11, %ymm7, %ymm7
        vpmuldq %ymm12, %ymm8, %ymm8
        vpmuldq %ymm13, %ymm9, %ymm9
.endm

.macro acc
        vpaddq  %ymm2, %ymm6, %ymm2
        vpaddq  %ymm3, %ymm7, %ymm3
        vpaddq  %ymm4, %ymm8, %ymm4
        vpaddq  %ymm5, %ymm9, %ymm5
.endm

S2N_BN_SYMBOL(mldsa_pointwise_acc_l7_x86):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_DEC_RSP(176)
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
        movq    %rcx, %rdi
        movq    %rdx, %rsi
        movq    %r8, %rdx
        movq    %r9, %rcx
#endif

// Load constants from qdata
        vmovdqa (MLD_AVX2_BACKEND_DATA_OFFSET_8XQINV)*4(qdata), %ymm0
        vmovdqa (MLD_AVX2_BACKEND_DATA_OFFSET_8XQ)*4(qdata), %ymm1

        xorl    %eax, %eax
Lpointwise_acc_l7_looptop:
        pointwise 0

// Move first product
        vmovdqa %ymm6, %ymm2
        vmovdqa %ymm7, %ymm3
        vmovdqa %ymm8, %ymm4
        vmovdqa %ymm9, %ymm5

        pointwise 1024
        acc

        pointwise 2048
        acc

        pointwise 3072
        acc

        pointwise 4096
        acc

        pointwise 5120
        acc

        pointwise 6144
        acc

// Montgomery reduction
        vpmuldq %ymm2, %ymm0, %ymm6
        vpmuldq %ymm3, %ymm0, %ymm7
        vpmuldq %ymm4, %ymm0, %ymm8
        vpmuldq %ymm5, %ymm0, %ymm9
        vpmuldq %ymm6, %ymm1, %ymm6
        vpmuldq %ymm7, %ymm1, %ymm7
        vpmuldq %ymm8, %ymm1, %ymm8
        vpmuldq %ymm9, %ymm1, %ymm9
        vpsubq  %ymm6, %ymm2, %ymm2
        vpsubq  %ymm7, %ymm3, %ymm3
        vpsubq  %ymm8, %ymm4, %ymm4
        vpsubq  %ymm9, %ymm5, %ymm5

// Repack and store results
        vpsrlq  $32, %ymm2, %ymm2
        vmovshdup %ymm4, %ymm4
        vpblendd $0xAA, %ymm3, %ymm2, %ymm2
        vpblendd $0xAA, %ymm5, %ymm4, %ymm4

        vmovdqa %ymm2, (c)
        vmovdqa %ymm4, 32(c)

        addq    $64, a
        addq    $64, b
        addq    $64, c
        addl    $1, %eax
        cmpl    $16, %eax
        jb      Lpointwise_acc_l7_looptop

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
        CFI_INC_RSP(176)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_pointwise_acc_l7_x86)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
