// Copyright (c) The mldsa-native project authors
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Pointwise multiplication of polynomials in NTT domain (Montgomery form)
// Inputs a[256], b[256] (signed 32-bit words); output r[256] (signed 32-bit words)
//
// Performs pointwise multiplication of two polynomials in NTT domain using
// Montgomery arithmetic. The polynomials are represented as arrays of 256
// signed 32-bit coefficients.
//
// This computes r[i] = montgomery_reduce(a[i] * b[i]) for i = 0..255
//
// The Montgomery reduction is performed with:
//   Q = 8380417 (MLDSA prime modulus)
//   QINV = 58728449 (negative inverse of Q mod 2^32)
//   R = 2^32 (Montgomery parameter)
//
// extern void mldsa_pointwise
//      (int32_t r[static 256], const int32_t a[static 256],
//       const int32_t b[static 256]);
//
// Standard ARM ABI: X0 = r, X1 = a, X2 = b
// ----------------------------------------------------------------------------
#include "_internal_s2n_bignum_arm.h"

        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_pointwise)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_pointwise)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_pointwise)
        .text
        .balign 4

S2N_BN_SYMBOL(mldsa_pointwise):
        CFI_START

// This matches the code in the mldsa-native repository
// https://github.com/pq-code-package/mldsa-native/blob/main/mldsa/src/native/aarch64/src/pointwise_montgomery.S

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

Lmldsa_pointwise_loop:
        // Load 16 coefficients (4 vectors of 4 int32s) from a
        ldr     q17, [x1, #0x10]
        ldr     q18, [x1, #0x20]
        ldr     q19, [x1, #0x30]
        ldr     q16, [x1], #0x40
        
        // Load 16 coefficients (4 vectors of 4 int32s) from b
        ldr     q21, [x2, #0x10]
        ldr     q22, [x2, #0x20]
        ldr     q23, [x2, #0x30]
        ldr     q20, [x2], #0x40
        
        // Multiply a[i] * b[i] to get 64-bit products
        // smull extends 32-bit to 64-bit signed multiply (lower half)
        // smull2 extends 32-bit to 64-bit signed multiply (upper half)
        smull   v24.2d, v16.2s, v20.2s
        smull2  v25.2d, v16.4s, v20.4s
        smull   v26.2d, v17.2s, v21.2s
        smull2  v27.2d, v17.4s, v21.4s
        smull   v28.2d, v18.2s, v22.2s
        smull2  v29.2d, v18.4s, v22.4s
        smull   v30.2d, v19.2s, v23.2s
        smull2  v31.2d, v19.4s, v23.4s
        
        // Montgomery reduction for v16/v24-v25
        // Extract lower 32 bits, multiply by QINV
        uzp1    v16.4s, v24.4s, v25.4s
        mul     v16.4s, v16.4s, v1.4s
        // Subtract (lower_bits * QINV) * Q from 64-bit product
        smlsl   v24.2d, v16.2s, v0.2s
        smlsl2  v25.2d, v16.4s, v0.4s
        // Extract upper 32 bits (the reduced result)
        uzp2    v16.4s, v24.4s, v25.4s
        
        // Montgomery reduction for v17/v26-v27
        uzp1    v17.4s, v26.4s, v27.4s
        mul     v17.4s, v17.4s, v1.4s
        smlsl   v26.2d, v17.2s, v0.2s
        smlsl2  v27.2d, v17.4s, v0.4s
        uzp2    v17.4s, v26.4s, v27.4s
        
        // Montgomery reduction for v18/v28-v29
        uzp1    v18.4s, v28.4s, v29.4s
        mul     v18.4s, v18.4s, v1.4s
        smlsl   v28.2d, v18.2s, v0.2s
        smlsl2  v29.2d, v18.4s, v0.4s
        uzp2    v18.4s, v28.4s, v29.4s
        
        // Montgomery reduction for v19/v30-v31
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
        cbnz    x3, Lmldsa_pointwise_loop
        
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_pointwise)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack, "", %progbits
#endif
