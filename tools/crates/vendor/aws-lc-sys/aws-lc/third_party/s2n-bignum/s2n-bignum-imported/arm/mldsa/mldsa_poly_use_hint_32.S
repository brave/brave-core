// Copyright (c) The mldsa-native project authors
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Use hint to correct high bits of decomposition (parameter sets 65/87)
// Inputs a[256] (unsigned 32-bit, in [0,Q)), h[256] (hint bits, 0 or 1)
// Output b[256] (unsigned 32-bit, in [0,16))
//
// Implements mld_use_hint for ML-DSA parameter sets 65/87:
//   GAMMA2 = (Q-1)/32 = 261888
//   2*GAMMA2 = 523776
//   Output range: [0, 15]
//
// Algorithm per coefficient:
//   1. Decompose: a1 = round_down(a / 523776), a0 = a - a1*523776
//      If a > 31*GAMMA2 = 8118528, wrap: a1=0, a0=a-Q
//   2. delta = (a0 <= 0) ? -1 : 1
//   3. b = (a1 + delta * h) & 15
//
// extern void mldsa_poly_use_hint_32
//      (int32_t b[static 256], const int32_t a[static 256],
//       const int32_t h[static 256]);
//
// Standard ARM ABI: X0 = b, X1 = a, X2 = h
// ----------------------------------------------------------------------------
#include "_internal_s2n_bignum_arm.h"

        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_poly_use_hint_32)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_poly_use_hint_32)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_poly_use_hint_32)
        .text
        .balign 4

S2N_BN_SYMBOL(mldsa_poly_use_hint_32):
        CFI_START

// This matches the code in the mldsa-native repository
// https://github.com/pq-code-package/mldsa-native/blob/main/mldsa/src/native/aarch64/src/poly_use_hint_32_aarch64_asm.S

// Load constants into SIMD registers

// v20 = Q = 8380417 (unused in computation but part of original code)
        mov     w4, #0xe001
        movk    w4, #0x7f, lsl #16
        dup     v20.4s, w4

// v21 = 31*GAMMA2 = 8118528 = 0x7be100 (wraparound threshold)
        mov     w5, #0xe100
        movk    w5, #0x7b, lsl #16
        dup     v21.4s, w5

// v22 = 2*GAMMA2 = 523776 = 0x7fe00 (decompose multiplier)
        mov     w7, #0xfe00
        movk    w7, #0x7, lsl #16
        dup     v22.4s, w7

// v23 = Barrett constant = 0x40100401 = 1074791425
// Used for SQDMULH-based Barrett reduction: a1 ~= (2*a*c) >> 49
        mov     w11, #0x0401
        movk    w11, #0x4010, lsl #16
        dup     v23.4s, w11

// v24 = mask 15 = 0x0000000f (for final AND to compute mod 16)
        movi    v24.4s, #0xf

// Loop counter: 16 iterations, processing 16 coefficients per iteration
// 16 * 16 = 256 total coefficients
        mov     x3, #0x10

Lmldsa_poly_use_hint_32_loop:
        // Load 16 coefficients from a (4 vectors of 4 int32s)
        ldr     q1, [x1, #0x10]
        ldr     q2, [x1, #0x20]
        ldr     q3, [x1, #0x30]
        ldr     q0, [x1], #0x40

        // Load 16 hint bits from h (4 vectors of 4 int32s)
        ldr     q5, [x2, #0x10]
        ldr     q6, [x2, #0x20]
        ldr     q7, [x2, #0x30]
        ldr     q4, [x2], #0x40

        // --- Process v1 (coefficients at offset +16) ---
        // Decompose: a1 = sqdmulh(a, barrett_const) >> 18
        sqdmulh v17.4s, v1.4s, v23.4s
        srshr   v17.4s, v17.4s, #0x12
        // Check wraparound: mask = (a > 31*GAMMA2) ? all_ones : 0
        cmgt    v25.4s, v1.4s, v21.4s
        // a0 = a - a1 * 2*GAMMA2
        mls     v1.4s, v17.4s, v22.4s
        // If wraparound: a1 = 0 (clear a1 where mask is set)
        bic     v17.16b, v17.16b, v25.16b
        // If wraparound: a0 -= 1 (add all_ones = -1)
        add     v1.4s, v1.4s, v25.4s
        // delta = (a0 <= 0) ? all_ones : 0
        cmle    v1.4s, v1.4s, #0
        // delta = (a0 <= 0) ? -1 : 1 (set bit 0)
        orr     v1.4s, #0x1
        // b = a1 + delta * hint
        mla     v17.4s, v1.4s, v5.4s
        // b = b & 15
        and     v17.16b, v17.16b, v24.16b

        // --- Process v2 (coefficients at offset +32) ---
        sqdmulh v18.4s, v2.4s, v23.4s
        srshr   v18.4s, v18.4s, #0x12
        cmgt    v25.4s, v2.4s, v21.4s
        mls     v2.4s, v18.4s, v22.4s
        bic     v18.16b, v18.16b, v25.16b
        add     v2.4s, v2.4s, v25.4s
        cmle    v2.4s, v2.4s, #0
        orr     v2.4s, #0x1
        mla     v18.4s, v2.4s, v6.4s
        and     v18.16b, v18.16b, v24.16b

        // --- Process v3 (coefficients at offset +48) ---
        sqdmulh v19.4s, v3.4s, v23.4s
        srshr   v19.4s, v19.4s, #0x12
        cmgt    v25.4s, v3.4s, v21.4s
        mls     v3.4s, v19.4s, v22.4s
        bic     v19.16b, v19.16b, v25.16b
        add     v3.4s, v3.4s, v25.4s
        cmle    v3.4s, v3.4s, #0
        orr     v3.4s, #0x1
        mla     v19.4s, v3.4s, v7.4s
        and     v19.16b, v19.16b, v24.16b

        // --- Process v0 (coefficients at offset +0, loaded last for post-increment) ---
        sqdmulh v16.4s, v0.4s, v23.4s
        srshr   v16.4s, v16.4s, #0x12
        cmgt    v25.4s, v0.4s, v21.4s
        mls     v0.4s, v16.4s, v22.4s
        bic     v16.16b, v16.16b, v25.16b
        add     v0.4s, v0.4s, v25.4s
        cmle    v0.4s, v0.4s, #0
        orr     v0.4s, #0x1
        mla     v16.4s, v0.4s, v4.4s
        and     v16.16b, v16.16b, v24.16b

        // Store 16 output coefficients
        str     q17, [x0, #0x10]
        str     q18, [x0, #0x20]
        str     q19, [x0, #0x30]
        str     q16, [x0], #0x40

        // Decrement loop counter and branch
        subs    x3, x3, #0x1
        b.ne    Lmldsa_poly_use_hint_32_loop

        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_poly_use_hint_32)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack, "", %progbits
#endif
