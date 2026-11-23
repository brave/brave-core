// Copyright (c) The mldsa-native project authors
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Coefficient decomposition for ML-DSA (parameter set 44, GAMMA2 = (Q-1)/88)
// For each coefficient a in [0,Q), computes (a1, a0) with a = a1*2*GAMMA2 + a0.
// Input a[256] (signed 32-bit words); outputs r1[256] (high parts) and, in
// place of a, the low parts a0.
//
// extern void mldsa_decompose_88(int32_t a1[static 256], int32_t a0[static 256]);
//
// Standard ARM ABI: X0 = r1, X1 = a
// ----------------------------------------------------------------------------
#include "_internal_s2n_bignum_arm.h"

        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_decompose_88)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_decompose_88)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_decompose_88)
        .text
        .balign 4

S2N_BN_SYMBOL(mldsa_decompose_88):

        mov w4, #0xe001             // =57345
        movk w4, #0x7f, lsl #16
        dup v20.4s, w4
        mov w5, #0x6c00             // =27648
        movk w5, #0x7e, lsl #16
        dup v21.4s, w5
        mov w7, #0xe800             // =59392
        movk w7, #0x2, lsl #16
        dup v22.4s, w7
        mov w11, #0x581             // =1409
        movk w11, #0x5816, lsl #16
        dup v23.4s, w11
        mov x3, #0x10               // =16

Lmldsa_decompose_88_loop:
        ldr q0, [x1]
        ldr q1, [x1, #0x10]
        ldr q2, [x1, #0x20]
        ldr q3, [x1, #0x30]
        sqdmulh v5.4s, v1.4s, v23.4s
        srshr v5.4s, v5.4s, #0x11
        cmgt v24.4s, v1.4s, v21.4s
        mls v1.4s, v5.4s, v22.4s
        bic v5.16b, v5.16b, v24.16b
        add v1.4s, v1.4s, v24.4s
        sqdmulh v6.4s, v2.4s, v23.4s
        srshr v6.4s, v6.4s, #0x11
        cmgt v24.4s, v2.4s, v21.4s
        mls v2.4s, v6.4s, v22.4s
        bic v6.16b, v6.16b, v24.16b
        add v2.4s, v2.4s, v24.4s
        sqdmulh v7.4s, v3.4s, v23.4s
        srshr v7.4s, v7.4s, #0x11
        cmgt v24.4s, v3.4s, v21.4s
        mls v3.4s, v7.4s, v22.4s
        bic v7.16b, v7.16b, v24.16b
        add v3.4s, v3.4s, v24.4s
        sqdmulh v4.4s, v0.4s, v23.4s
        srshr v4.4s, v4.4s, #0x11
        cmgt v24.4s, v0.4s, v21.4s
        mls v0.4s, v4.4s, v22.4s
        bic v4.16b, v4.16b, v24.16b
        add v0.4s, v0.4s, v24.4s
        str q5, [x0, #0x10]
        str q6, [x0, #0x20]
        str q7, [x0, #0x30]
        str q4, [x0], #0x40
        str q1, [x1, #0x10]
        str q2, [x1, #0x20]
        str q3, [x1, #0x30]
        str q0, [x1], #0x40
        subs x3, x3, #0x1
        b.ne Lmldsa_decompose_88_loop
        ret

S2N_BN_SIZE_DIRECTIVE(mldsa_decompose_88)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack, "", %progbits
#endif
