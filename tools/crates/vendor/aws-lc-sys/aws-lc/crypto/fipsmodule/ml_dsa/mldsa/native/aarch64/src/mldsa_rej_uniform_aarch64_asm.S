/*
 * Copyright (c) The mldsa-native project authors
 * Copyright (c) The mlkem-native project authors
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */

/*yaml
  Name: rej_uniform_aarch64_asm
  Description: AArch64 rejection sampling of uniform coefficients mod q
  Signature: uint64_t mld_rej_uniform_aarch64_asm(int32_t r[256], const uint8_t *buf, unsigned buflen, const uint8_t table[256])
  ABI:
    Architecture: aarch64
    CallingConvention: AAPCS64
    Features: [NEON]
    x0:
      type: buffer
      size_bytes: 1024
      permissions: write-only
      c_parameter: int32_t r[256]
      description: Output buffer (256 x int32_t)
    x1:
      type: buffer
      size_bytes: x2
      permissions: read-only
      c_parameter: const uint8_t *buf
      description: Input buffer
    x2:
      type: scalar
      c_parameter: unsigned buflen
      description: Length of input buffer (must be a multiple of 24)
      test_with: 840  # MLD_POLY_UNIFORM_NBLOCKS * SHAKE128_RATE = 5 * 168
    x3:
      type: buffer
      size_bytes: 256
      permissions: read-only
      c_parameter: const uint8_t table[256]
      description: Lookup table (256 x uint8_t)
*/

 #include "_internal_s2n_bignum_arm.h"

/*
 * WARNING: This file is auto-derived from the mldsa-native source file
 *   dev/aarch64_opt/src/mldsa_rej_uniform_aarch64_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_rej_uniform_aarch64_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_rej_uniform_aarch64_asm)
S2N_BN_SYMBOL(mldsa_rej_uniform_aarch64_asm):

        .cfi_startproc
        sub sp, sp, #0x440
        .cfi_adjust_cfa_offset 0x440
        mov x7, #0x1                // =1
        movk x7, #0x2, lsl #32
        mov v31.d[0], x7
        mov x7, #0x4                // =4
        movk x7, #0x8, lsl #32
        mov v31.d[1], x7
        mov w7, #0xe001             // =57345
        movk w7, #0x7f, lsl #16
        dup v30.4s, w7
        mov x8, sp
        mov x7, x8
        mov x11, #0x0               // =0
        eor v16.16b, v16.16b, v16.16b

Lmld_rej_uniform_initial_zero:
        str q16, [x7], #0x40
        stur q16, [x7, #-0x30]
        stur q16, [x7, #-0x20]
        stur q16, [x7, #-0x10]
        add x11, x11, #0x10
        cmp x11, #0x100
        b.lt Lmld_rej_uniform_initial_zero
        mov x7, x8
        mov x9, #0x0                // =0
        mov x4, #0x100              // =256
        cmp x2, #0x30
        b.lo Lmld_rej_uniform_loop48_end

Lmld_rej_uniform_loop48:
        cmp x9, x4
        b.hs Lmld_rej_uniform_memory_copy
        sub x2, x2, #0x30
        ld3 { v0.16b, v1.16b, v2.16b }, [x1], #48
        movi v4.16b, #0x80
        bic v2.16b, v2.16b, v4.16b
        zip1 v4.16b, v0.16b, v1.16b
        zip2 v5.16b, v0.16b, v1.16b
        ushll v6.8h, v2.8b, #0x0
        ushll2 v7.8h, v2.16b, #0x0
        zip1 v16.8h, v4.8h, v6.8h
        zip2 v17.8h, v4.8h, v6.8h
        zip1 v18.8h, v5.8h, v7.8h
        zip2 v19.8h, v5.8h, v7.8h
        cmhi v4.4s, v30.4s, v16.4s
        cmhi v5.4s, v30.4s, v17.4s
        cmhi v6.4s, v30.4s, v18.4s
        cmhi v7.4s, v30.4s, v19.4s
        and v4.16b, v4.16b, v31.16b
        and v5.16b, v5.16b, v31.16b
        and v6.16b, v6.16b, v31.16b
        and v7.16b, v7.16b, v31.16b
        uaddlv d20, v4.4s
        uaddlv d21, v5.4s
        uaddlv d22, v6.4s
        uaddlv d23, v7.4s
        fmov x12, d20
        fmov x13, d21
        fmov x14, d22
        fmov x15, d23
        ldr q24, [x3, x12, lsl #4]
        ldr q25, [x3, x13, lsl #4]
        ldr q26, [x3, x14, lsl #4]
        ldr q27, [x3, x15, lsl #4]
        cnt v4.16b, v4.16b
        cnt v5.16b, v5.16b
        cnt v6.16b, v6.16b
        cnt v7.16b, v7.16b
        uaddlv d20, v4.4s
        uaddlv d21, v5.4s
        uaddlv d22, v6.4s
        uaddlv d23, v7.4s
        fmov x12, d20
        fmov x13, d21
        fmov x14, d22
        fmov x15, d23
        tbl v16.16b, { v16.16b }, v24.16b
        tbl v17.16b, { v17.16b }, v25.16b
        tbl v18.16b, { v18.16b }, v26.16b
        tbl v19.16b, { v19.16b }, v27.16b
        st1 { v16.4s }, [x7]
        add x7, x7, x12, lsl #2
        st1 { v17.4s }, [x7]
        add x7, x7, x13, lsl #2
        st1 { v18.4s }, [x7]
        add x7, x7, x14, lsl #2
        st1 { v19.4s }, [x7]
        add x7, x7, x15, lsl #2
        add x12, x12, x13
        add x14, x14, x15
        add x9, x9, x12
        add x9, x9, x14
        cmp x2, #0x30
        b.hs Lmld_rej_uniform_loop48

Lmld_rej_uniform_loop48_end:
        cmp x9, x4
        b.hs Lmld_rej_uniform_memory_copy
        cmp x2, #0x18
        b.lo Lmld_rej_uniform_memory_copy
        sub x2, x2, #0x18
        ld3 { v0.8b, v1.8b, v2.8b }, [x1], #24
        movi v4.16b, #0x80
        bic v2.16b, v2.16b, v4.16b
        zip1 v4.16b, v0.16b, v1.16b
        ushll v6.8h, v2.8b, #0x0
        zip1 v16.8h, v4.8h, v6.8h
        zip2 v17.8h, v4.8h, v6.8h
        cmhi v4.4s, v30.4s, v16.4s
        cmhi v5.4s, v30.4s, v17.4s
        and v4.16b, v4.16b, v31.16b
        and v5.16b, v5.16b, v31.16b
        uaddlv d20, v4.4s
        uaddlv d21, v5.4s
        fmov x12, d20
        fmov x13, d21
        ldr q24, [x3, x12, lsl #4]
        ldr q25, [x3, x13, lsl #4]
        cnt v4.16b, v4.16b
        cnt v5.16b, v5.16b
        uaddlv d20, v4.4s
        uaddlv d21, v5.4s
        fmov x12, d20
        fmov x13, d21
        tbl v16.16b, { v16.16b }, v24.16b
        tbl v17.16b, { v17.16b }, v25.16b
        st1 { v16.4s }, [x7]
        add x7, x7, x12, lsl #2
        st1 { v17.4s }, [x7]
        add x7, x7, x13, lsl #2
        add x9, x9, x12
        add x9, x9, x13

Lmld_rej_uniform_memory_copy:
        cmp x9, x4
        csel x9, x9, x4, lo
        mov x11, #0x0               // =0
        mov x7, x8

Lmld_rej_uniform_final_copy:
        ldr q16, [x7], #0x40
        ldur q17, [x7, #-0x30]
        ldur q18, [x7, #-0x20]
        ldur q19, [x7, #-0x10]
        str q16, [x0], #0x40
        stur q17, [x0, #-0x30]
        stur q18, [x0, #-0x20]
        stur q19, [x0, #-0x10]
        add x11, x11, #0x10
        cmp x11, #0x100
        b.lt Lmld_rej_uniform_final_copy
        mov x0, x9
        b Lmld_rej_uniform_return

Lmld_rej_uniform_return:
        add sp, sp, #0x440
        .cfi_adjust_cfa_offset -0x440
        ret
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_rej_uniform_aarch64_asm)


#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
