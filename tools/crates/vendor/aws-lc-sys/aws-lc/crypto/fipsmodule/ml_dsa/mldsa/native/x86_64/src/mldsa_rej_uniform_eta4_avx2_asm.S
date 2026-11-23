/*
 * Copyright (c) The mldsa-native project authors
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */

/* References
 * ==========
 *
 * - [REF_AVX2]
 *   CRYSTALS-Dilithium optimized AVX2 implementation
 *   Bai, Ducas, Kiltz, Lepoint, Lyubashevsky, Schwabe, Seiler, Stehlé
 *   https://github.com/pq-crystals/dilithium/tree/master/avx2
 */

/*
 * This file is derived from the public domain
 * AVX2 Dilithium implementation @[REF_AVX2].
 */

/*yaml
  Name: rej_uniform_eta4_avx2_asm
  Description: x86_64 AVX2 rejection sampling of eta=4 secret coefficients.
    Extracts 4-bit nibbles from the input byte buffer, accepts those < 9,
    and maps an accepted nibble n to the coefficient 4 - n, producing
    values in [-4, 4].
  Signature: unsigned mld_rej_uniform_eta4_avx2_asm(int32_t *r, const uint8_t *buf, const uint8_t table[256][8])
  ABI:
    Architecture: x86_64
    CallingConvention: SysV
    Features: [AVX2]
    rdi:
      type: buffer
      size_bytes: 1024
      permissions: write-only
      c_parameter: int32_t *r
      description: Output coefficient buffer (256 x int32_t)
    rsi:
      type: buffer
      size_bytes: 272
      permissions: read-only
      c_parameter: const uint8_t *buf
      description: Input byte buffer (MLD_AVX2_REJ_UNIFORM_ETA4_BUFLEN = 272)
    rdx:
      type: buffer
      size_bytes: 2048
      permissions: read-only
      c_parameter: const uint8_t *table
      description: Lookup table (256 x 8 uint8_t = mld_rej_uniform_table)
*/

#include "_internal_s2n_bignum_x86_att.h"

/*
 * WARNING: This file is auto-derived from the mldsa-native source file
 *   dev/x86_64/src/mldsa_rej_uniform_eta4_avx2_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_rej_uniform_eta4_avx2_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_rej_uniform_eta4_avx2_asm)
S2N_BN_SYMBOL(mldsa_rej_uniform_eta4_avx2_asm):

        .cfi_startproc
        movl $0xf0f0f0f, %r8d        # imm = 0xF0F0F0F
        vmovd %r8d, %xmm2
        vpbroadcastd %xmm2, %ymm2
        movl $0x4040404, %r8d        # imm = 0x4040404
        vmovd %r8d, %xmm3
        vpbroadcastd %xmm3, %ymm3
        movl $0x9090909, %r8d        # imm = 0x9090909
        vmovd %r8d, %xmm4
        vpbroadcastd %xmm4, %ymm4
        xorl %eax, %eax
        xorl %ecx, %ecx

Lmld_rej_uniform_eta4_avx2_asm_loop:
        cmpl $0xf8, %eax
        ja Lmld_rej_uniform_eta4_avx2_asm_scalar
        cmpl $0x100, %ecx            # imm = 0x100
        ja Lmld_rej_uniform_eta4_avx2_asm_scalar
        vpmovzxbw (%rsi,%rcx), %ymm0
        vpsllw $0x4, %ymm0, %ymm1
        vpor %ymm1, %ymm0, %ymm0
        vpand %ymm2, %ymm0, %ymm0
        vpsubb %ymm4, %ymm0, %ymm1
        vpsubb %ymm0, %ymm3, %ymm0
        vpmovmskb %ymm1, %r8d
        vextracti128 $0x0, %ymm0, %xmm5
        movzbl %r8b, %r10d
        vmovq (%rdx,%r10,8), %xmm6
        vpshufb %xmm6, %xmm5, %xmm6
        vpmovsxbd %xmm6, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcntl %r10d, %r9d
        addl %r9d, %eax
        shrl $0x8, %r8d
        addl $0x4, %ecx
        cmpl $0xf8, %eax
        ja Lmld_rej_uniform_eta4_avx2_asm_scalar
        vpsrldq $0x8, %xmm5, %xmm5      # xmm5 = xmm5[8,9,10,11,12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero
        movzbl %r8b, %r10d
        vmovq (%rdx,%r10,8), %xmm6
        vpshufb %xmm6, %xmm5, %xmm6
        vpmovsxbd %xmm6, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcntl %r10d, %r9d
        addl %r9d, %eax
        shrl $0x8, %r8d
        addl $0x4, %ecx
        cmpl $0xf8, %eax
        ja Lmld_rej_uniform_eta4_avx2_asm_scalar
        vextracti128 $0x1, %ymm0, %xmm5
        movzbl %r8b, %r10d
        vmovq (%rdx,%r10,8), %xmm6
        vpshufb %xmm6, %xmm5, %xmm6
        vpmovsxbd %xmm6, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcntl %r10d, %r9d
        addl %r9d, %eax
        shrl $0x8, %r8d
        addl $0x4, %ecx
        cmpl $0xf8, %eax
        ja Lmld_rej_uniform_eta4_avx2_asm_scalar
        vpsrldq $0x8, %xmm5, %xmm5      # xmm5 = xmm5[8,9,10,11,12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero
        movzbl %r8b, %r10d
        vmovq (%rdx,%r10,8), %xmm6
        vpshufb %xmm6, %xmm5, %xmm6
        vpmovsxbd %xmm6, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcntl %r10d, %r9d
        addl %r9d, %eax
        addl $0x4, %ecx
        jmp Lmld_rej_uniform_eta4_avx2_asm_loop

Lmld_rej_uniform_eta4_avx2_asm_scalar:
        cmpl $0x100, %eax            # imm = 0x100
        jae Lmld_rej_uniform_eta4_avx2_asm_done
        cmpl $0x110, %ecx            # imm = 0x110
        jae Lmld_rej_uniform_eta4_avx2_asm_done
        movzbl (%rsi,%rcx), %r11d
        incl %ecx
        movl %r11d, %r10d
        andl $0xf, %r10d
        cmpl $0x9, %r10d
        jae Lmld_rej_uniform_eta4_avx2_asm_high_nibble
        movl $0x4, %r9d
        subl %r10d, %r9d
        movl %r9d, (%rdi,%rax,4)
        incl %eax
        cmpl $0x100, %eax            # imm = 0x100
        jae Lmld_rej_uniform_eta4_avx2_asm_done

Lmld_rej_uniform_eta4_avx2_asm_high_nibble:
        shrl $0x4, %r11d
        andl $0xf, %r11d
        cmpl $0x9, %r11d
        jae Lmld_rej_uniform_eta4_avx2_asm_scalar
        movl $0x4, %r10d
        subl %r11d, %r10d
        movl %r10d, (%rdi,%rax,4)
        incl %eax
        jmp Lmld_rej_uniform_eta4_avx2_asm_scalar

Lmld_rej_uniform_eta4_avx2_asm_done:
        retq
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_rej_uniform_eta4_avx2_asm)


#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
