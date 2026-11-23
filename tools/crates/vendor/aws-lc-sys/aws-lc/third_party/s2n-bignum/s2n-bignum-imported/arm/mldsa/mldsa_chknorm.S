// Copyright (c) The mldsa-native project authors
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Infinity-norm check of polynomial coefficients for ML-DSA
// Returns 1 if any coefficient has absolute value >= bound, 0 otherwise.
// Input a[256] (signed 32-bit words), bound (unsigned 32-bit).
//
// extern uint64_t mldsa_chknorm(const int32_t a[static 256], uint64_t bound);
//
// Standard ARM ABI: X0 = a, W1 = bound; result in W0
// ----------------------------------------------------------------------------
#include "_internal_s2n_bignum_arm.h"

        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_chknorm)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_chknorm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_chknorm)
        .text
        .balign 4

S2N_BN_SYMBOL(mldsa_chknorm):

        dup v20.4s, w1
        eor v21.16b, v21.16b, v21.16b
        mov x2, #0x10               // =16

Lmldsa_chknorm_loop:
        ldr q1, [x0, #0x10]
        ldr q2, [x0, #0x20]
        ldr q3, [x0, #0x30]
        ldr q0, [x0], #0x40
        abs v1.4s, v1.4s
        cmge v1.4s, v1.4s, v20.4s
        orr v21.16b, v21.16b, v1.16b
        abs v2.4s, v2.4s
        cmge v2.4s, v2.4s, v20.4s
        orr v21.16b, v21.16b, v2.16b
        abs v3.4s, v3.4s
        cmge v3.4s, v3.4s, v20.4s
        orr v21.16b, v21.16b, v3.16b
        abs v0.4s, v0.4s
        cmge v0.4s, v0.4s, v20.4s
        orr v21.16b, v21.16b, v0.16b
        subs x2, x2, #0x1
        b.ne Lmldsa_chknorm_loop
        umaxv s21, v21.4s
        fmov w0, s21
        and w0, w0, #0x1
        ret

S2N_BN_SIZE_DIRECTIVE(mldsa_chknorm)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack, "", %progbits
#endif
