// Copyright (c) The mldsa-native project authors
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Unpack the packed z polynomial for ML-DSA (GAMMA1 = 2^19, parameter sets 65/87)
// Reads 20-bit packed coefficients from the byte buffer b and produces
// r[256] = GAMMA1 - coeff (signed 32-bit words), using the shuffle table t.
//
// extern void mldsa_polyz_unpack_19_arm(int32_t r[static 256], const uint8_t b[static 640], const uint8_t t[static 64]);
//
// Standard ARM ABI: X0 = r, X1 = b, X2 = t
// ----------------------------------------------------------------------------
#include "_internal_s2n_bignum_arm.h"

        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_polyz_unpack_19_arm)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_polyz_unpack_19_arm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_polyz_unpack_19_arm)
        .text
        .balign 4

S2N_BN_SYMBOL(mldsa_polyz_unpack_19_arm):

        ldr q24, [x2]
        ldr q25, [x2, #0x10]
        ldr q26, [x2, #0x20]
        ldr q27, [x2, #0x30]
        mov x3, #0xfc00000000       // =1082331758592
        dup v28.2d, x3
        movi v29.4s, #0xf, msl #16
        movi v30.4s, #0x8, lsl #16
        mov x9, #0x10               // =16

Lmldsa_polyz_unpack_19_loop:
        ld1 { v0.16b, v1.16b }, [x1]
        add x1, x1, #0x18
        ld1 { v2.16b }, [x1], #16
        tbl v4.16b, { v0.16b }, v24.16b
        tbl v5.16b, { v0.16b, v1.16b }, v25.16b
        tbl v6.16b, { v1.16b }, v26.16b
        tbl v7.16b, { v1.16b, v2.16b }, v27.16b
        ushl v4.4s, v4.4s, v28.4s
        and v4.16b, v4.16b, v29.16b
        sub v4.4s, v30.4s, v4.4s
        ushl v5.4s, v5.4s, v28.4s
        and v5.16b, v5.16b, v29.16b
        sub v5.4s, v30.4s, v5.4s
        ushl v6.4s, v6.4s, v28.4s
        and v6.16b, v6.16b, v29.16b
        sub v6.4s, v30.4s, v6.4s
        ushl v7.4s, v7.4s, v28.4s
        and v7.16b, v7.16b, v29.16b
        sub v7.4s, v30.4s, v7.4s
        str q5, [x0, #0x10]
        str q6, [x0, #0x20]
        str q7, [x0, #0x30]
        str q4, [x0], #0x40
        subs x9, x9, #0x1
        b.ne Lmldsa_polyz_unpack_19_loop
        ret

S2N_BN_SIZE_DIRECTIVE(mldsa_polyz_unpack_19_arm)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack, "", %progbits
#endif
