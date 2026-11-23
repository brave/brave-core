// Copyright (c) The mldsa-native project authors
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Pointwise multiplication and accumulation of polynomials in ML-DSA NTT
// Inputs a[1792], b[1792] (signed 32-bit words); output r[256] (signed 32-bit words)
//
// Performs pointwise multiply-accumulate of 7 polynomial pairs in NTT domain
// using Montgomery arithmetic:
//   r[i] = montgomery_reduce(sum_{k=0}^{6} a[i + 256*k] * b[i + 256*k])
// for i = 0..255
//
// The Montgomery reduction is performed with:
//   Q = 8380417 (MLDSA prime modulus)
//   QINV = 58728449 (negative inverse of Q mod 2^32)
//   R = 2^32 (Montgomery parameter)
//
// Input bounds (per coefficient, absolute value):
//   |a[i]| < Q
//   |b[i]| < 9*Q
// Output coefficients are bounded by Q in absolute value.
//
// extern void mldsa_pointwise_acc_l7
//      (int32_t r[static 256], const int32_t a[static 1792],
//       const int32_t b[static 1792]);
//
// Standard ARM ABI: X0 = r, X1 = a, X2 = b
// ----------------------------------------------------------------------------
#include "_internal_s2n_bignum_arm.h"

        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_pointwise_acc_l7)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_pointwise_acc_l7)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_pointwise_acc_l7)
        .text
        .balign 4

S2N_BN_SYMBOL(mldsa_pointwise_acc_l7):
        CFI_START

// This matches the code in the mldsa-native repository
// https://github.com/pq-code-package/mldsa-native/blob/main/mldsa/src/native/aarch64/src/mld_polyvecl_pointwise_acc_montgomery_l7.S

// Load Q = 8380417 = 0x7fe001 into v0.4s
// This is the MLDSA prime modulus
        mov     w3, #0xe001             // Lower 16 bits: 57345
        movk    w3, #0x7f, lsl #16      // Upper 16 bits: 127, giving 0x7fe001
        dup     v0.4s, w3

// Load QINV = 58728449 = 0x3802001 into v1.4s
// This is the negative inverse of Q modulo 2^32
        mov     w3, #0x2001             // Lower 16 bits: 8193
        movk    w3, #0x380, lsl #16     // Upper 16 bits: 896, giving 0x3802001
        dup     v1.4s, w3

// Loop counter: 64 iterations, processing 4 coefficients per iteration
// 64 * 4 = 256 total coefficients
        mov     x3, #0x40               // =64

Lmldsa_pointwise_acc_l7_loop:

        // ---- Polynomial pair 0: a[i]*b[i] (smull = initial multiply) ----
        ldr     q17, [x1, #0x10]
        ldr     q18, [x1, #0x20]
        ldr     q19, [x1, #0x30]
        ldr     q16, [x1], #0x40
        ldr     q21, [x2, #0x10]
        ldr     q22, [x2, #0x20]
        ldr     q23, [x2, #0x30]
        ldr     q20, [x2], #0x40

        smull   v24.2d, v16.2s, v20.2s
        smull2  v25.2d, v16.4s, v20.4s
        smull   v26.2d, v17.2s, v21.2s
        smull2  v27.2d, v17.4s, v21.4s
        smull   v28.2d, v18.2s, v22.2s
        smull2  v29.2d, v18.4s, v22.4s
        smull   v30.2d, v19.2s, v23.2s
        smull2  v31.2d, v19.4s, v23.4s

        // ---- Polynomial pair 1: accumulate a[i+256]*b[i+256] ----
        // Offset 0x3c0 = 1024 - 64 (since x1 already advanced by 0x40)
        ldr     q16, [x1, #0x3c0]
        ldr     q17, [x1, #0x3d0]
        ldr     q18, [x1, #0x3e0]
        ldr     q19, [x1, #0x3f0]
        ldr     q20, [x2, #0x3c0]
        ldr     q21, [x2, #0x3d0]
        ldr     q22, [x2, #0x3e0]
        ldr     q23, [x2, #0x3f0]

        smlal   v24.2d, v16.2s, v20.2s
        smlal2  v25.2d, v16.4s, v20.4s
        smlal   v26.2d, v17.2s, v21.2s
        smlal2  v27.2d, v17.4s, v21.4s
        smlal   v28.2d, v18.2s, v22.2s
        smlal2  v29.2d, v18.4s, v22.4s
        smlal   v30.2d, v19.2s, v23.2s
        smlal2  v31.2d, v19.4s, v23.4s

        // ---- Polynomial pair 2: accumulate a[i+512]*b[i+512] ----
        // Offset 0x7c0 = 2048 - 64
        ldr     q16, [x1, #0x7c0]
        ldr     q17, [x1, #0x7d0]
        ldr     q18, [x1, #0x7e0]
        ldr     q19, [x1, #0x7f0]
        ldr     q20, [x2, #0x7c0]
        ldr     q21, [x2, #0x7d0]
        ldr     q22, [x2, #0x7e0]
        ldr     q23, [x2, #0x7f0]

        smlal   v24.2d, v16.2s, v20.2s
        smlal2  v25.2d, v16.4s, v20.4s
        smlal   v26.2d, v17.2s, v21.2s
        smlal2  v27.2d, v17.4s, v21.4s
        smlal   v28.2d, v18.2s, v22.2s
        smlal2  v29.2d, v18.4s, v22.4s
        smlal   v30.2d, v19.2s, v23.2s
        smlal2  v31.2d, v19.4s, v23.4s

        // ---- Polynomial pair 3: accumulate a[i+768]*b[i+768] ----
        // Offset 0xbc0 = 3072 - 64
        ldr     q16, [x1, #0xbc0]
        ldr     q17, [x1, #0xbd0]
        ldr     q18, [x1, #0xbe0]
        ldr     q19, [x1, #0xbf0]
        ldr     q20, [x2, #0xbc0]
        ldr     q21, [x2, #0xbd0]
        ldr     q22, [x2, #0xbe0]
        ldr     q23, [x2, #0xbf0]

        smlal   v24.2d, v16.2s, v20.2s
        smlal2  v25.2d, v16.4s, v20.4s
        smlal   v26.2d, v17.2s, v21.2s
        smlal2  v27.2d, v17.4s, v21.4s
        smlal   v28.2d, v18.2s, v22.2s
        smlal2  v29.2d, v18.4s, v22.4s
        smlal   v30.2d, v19.2s, v23.2s
        smlal2  v31.2d, v19.4s, v23.4s

        // ---- Polynomial pair 4: accumulate a[i+1024]*b[i+1024] ----
        // Offset 0xfc0 = 4096 - 64
        ldr     q16, [x1, #0xfc0]
        ldr     q17, [x1, #0xfd0]
        ldr     q18, [x1, #0xfe0]
        ldr     q19, [x1, #0xff0]
        ldr     q20, [x2, #0xfc0]
        ldr     q21, [x2, #0xfd0]
        ldr     q22, [x2, #0xfe0]
        ldr     q23, [x2, #0xff0]

        smlal   v24.2d, v16.2s, v20.2s
        smlal2  v25.2d, v16.4s, v20.4s
        smlal   v26.2d, v17.2s, v21.2s
        smlal2  v27.2d, v17.4s, v21.4s
        smlal   v28.2d, v18.2s, v22.2s
        smlal2  v29.2d, v18.4s, v22.4s
        smlal   v30.2d, v19.2s, v23.2s
        smlal2  v31.2d, v19.4s, v23.4s

        // ---- Polynomial pair 5: accumulate a[i+1280]*b[i+1280] ----
        // Offset 0x13c0 = 5120 - 64
        ldr     q16, [x1, #0x13c0]
        ldr     q17, [x1, #0x13d0]
        ldr     q18, [x1, #0x13e0]
        ldr     q19, [x1, #0x13f0]
        ldr     q20, [x2, #0x13c0]
        ldr     q21, [x2, #0x13d0]
        ldr     q22, [x2, #0x13e0]
        ldr     q23, [x2, #0x13f0]

        smlal   v24.2d, v16.2s, v20.2s
        smlal2  v25.2d, v16.4s, v20.4s
        smlal   v26.2d, v17.2s, v21.2s
        smlal2  v27.2d, v17.4s, v21.4s
        smlal   v28.2d, v18.2s, v22.2s
        smlal2  v29.2d, v18.4s, v22.4s
        smlal   v30.2d, v19.2s, v23.2s
        smlal2  v31.2d, v19.4s, v23.4s

        // ---- Polynomial pair 6: accumulate a[i+1536]*b[i+1536] ----
        // Offset 0x17c0 = 6144 - 64
        ldr     q16, [x1, #0x17c0]
        ldr     q17, [x1, #0x17d0]
        ldr     q18, [x1, #0x17e0]
        ldr     q19, [x1, #0x17f0]
        ldr     q20, [x2, #0x17c0]
        ldr     q21, [x2, #0x17d0]
        ldr     q22, [x2, #0x17e0]
        ldr     q23, [x2, #0x17f0]

        smlal   v24.2d, v16.2s, v20.2s
        smlal2  v25.2d, v16.4s, v20.4s
        smlal   v26.2d, v17.2s, v21.2s
        smlal2  v27.2d, v17.4s, v21.4s
        smlal   v28.2d, v18.2s, v22.2s
        smlal2  v29.2d, v18.4s, v22.4s
        smlal   v30.2d, v19.2s, v23.2s
        smlal2  v31.2d, v19.4s, v23.4s

        // ---- Montgomery reduction ----
        // For each group: extract low 32 bits, multiply by QINV,
        // subtract (low * QINV) * Q from 64-bit accumulator,
        // extract upper 32 bits as result

        // Montgomery reduction for v24-v25 -> v16
        uzp1    v16.4s, v24.4s, v25.4s
        mul     v16.4s, v16.4s, v1.4s
        smlsl   v24.2d, v16.2s, v0.2s
        smlsl2  v25.2d, v16.4s, v0.4s
        uzp2    v16.4s, v24.4s, v25.4s

        // Montgomery reduction for v26-v27 -> v17
        uzp1    v17.4s, v26.4s, v27.4s
        mul     v17.4s, v17.4s, v1.4s
        smlsl   v26.2d, v17.2s, v0.2s
        smlsl2  v27.2d, v17.4s, v0.4s
        uzp2    v17.4s, v26.4s, v27.4s

        // Montgomery reduction for v28-v29 -> v18
        uzp1    v18.4s, v28.4s, v29.4s
        mul     v18.4s, v18.4s, v1.4s
        smlsl   v28.2d, v18.2s, v0.2s
        smlsl2  v29.2d, v18.4s, v0.4s
        uzp2    v18.4s, v28.4s, v29.4s

        // Montgomery reduction for v30-v31 -> v19
        uzp1    v19.4s, v30.4s, v31.4s
        mul     v19.4s, v19.4s, v1.4s
        smlsl   v30.2d, v19.2s, v0.2s
        smlsl2  v31.2d, v19.4s, v0.4s
        uzp2    v19.4s, v30.4s, v31.4s

        // Store 16 reduced coefficients to output
        str     q17, [x0, #0x10]
        str     q18, [x0, #0x20]
        str     q19, [x0, #0x30]
        str     q16, [x0], #0x40

        // Decrement loop counter and continue if not zero
        subs    x3, x3, #0x4
        cbnz    x3, Lmldsa_pointwise_acc_l7_loop

        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_pointwise_acc_l7)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack, "", %progbits
#endif
