/*
 * Copyright (c) The mldsa-native project authors
 * Copyright (c) The mlkem-native project authors
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */

/*yaml
  Name: polyz_unpack_19_aarch64_asm
  Description: AArch64 unpacking of 19-bit packed coefficients
  Signature: void mld_polyz_unpack_19_aarch64_asm(int32_t r[256], const uint8_t buf[640], const uint8_t indices[64])
  ABI:
    Architecture: aarch64
    CallingConvention: AAPCS64
    Features: [NEON]
    x0:
      type: buffer
      size_bytes: 1024
      permissions: write-only
      c_parameter: int32_t r[256]
      description: Output polynomial (256 x int32_t)
    x1:
      type: buffer
      size_bytes: 640
      permissions: read-only
      c_parameter: const uint8_t buf[640]
      description: Packed input bytes
    x2:
      type: buffer
      size_bytes: 64
      permissions: read-only
      c_parameter: const uint8_t indices[64]
      description: Permutation index table (64 x uint8_t)
*/

 #include "_internal_s2n_bignum_arm.h"

/*
 * WARNING: This file is auto-derived from the mldsa-native source file
 *   dev/aarch64_opt/src/mldsa_polyz_unpack_19_aarch64_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_polyz_unpack_19_aarch64_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_polyz_unpack_19_aarch64_asm)
S2N_BN_SYMBOL(mldsa_polyz_unpack_19_aarch64_asm):

        .cfi_startproc
        ldr q24, [x2]
        ldr q25, [x2, #0x10]
        ldr q26, [x2, #0x20]
        ldr q27, [x2, #0x30]
        mov x3, #0xfc00000000       // =1082331758592
        dup v28.2d, x3
        movi v29.4s, #0xf, msl #16
        movi v30.4s, #0x8, lsl #16
        mov x9, #0x10               // =16

Lmld_polyz_unpack_19_loop:
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
        b.ne Lmld_polyz_unpack_19_loop
        ret
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_polyz_unpack_19_aarch64_asm)


#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
