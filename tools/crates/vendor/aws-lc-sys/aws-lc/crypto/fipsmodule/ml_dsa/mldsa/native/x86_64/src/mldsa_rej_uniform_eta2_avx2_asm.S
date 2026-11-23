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
  Name: rej_uniform_eta2_avx2_asm
  Description: x86_64 AVX2 rejection sampling of eta=2 secret coefficients.
    Extracts 4-bit nibbles from the input byte buffer, accepts those < 15,
    and maps an accepted nibble n to the coefficient 2 - (n mod 5) via a
    centered modulo-5 reduction, producing values in [-2, 2].
  Signature: unsigned mld_rej_uniform_eta2_avx2_asm(int32_t *r, const uint8_t *buf, const uint8_t table[256][8])
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
      size_bytes: 136
      permissions: read-only
      c_parameter: const uint8_t *buf
      description: Input byte buffer (MLD_AVX2_REJ_UNIFORM_ETA2_BUFLEN = 136)
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
 *   dev/x86_64/src/mldsa_rej_uniform_eta2_avx2_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_rej_uniform_eta2_avx2_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_rej_uniform_eta2_avx2_asm)
S2N_BN_SYMBOL(mldsa_rej_uniform_eta2_avx2_asm):

        .cfi_startproc
        movl $0xf0f0f0f, %r8d        # imm = 0xF0F0F0F
        vmovd %r8d, %xmm3
        vpbroadcastd %xmm3, %ymm3
        movl $0x2020202, %r8d        # imm = 0x2020202
        vmovd %r8d, %xmm4
        vpbroadcastd %xmm4, %ymm4
        movl $0xf0f0f0f, %r8d        # imm = 0xF0F0F0F
        vmovd %r8d, %xmm5
        vpbroadcastd %xmm5, %ymm5
        movl $0xffffe660, %r8d       # imm = 0xFFFFE660
        vpinsrw $0x0, %r8d, %xmm6, %xmm6
        vpbroadcastw %xmm6, %ymm6
        movl $0x5, %r8d
        vpinsrw $0x0, %r8d, %xmm7, %xmm7
        vpbroadcastw %xmm7, %ymm7
        xorl %eax, %eax
        xorl %ecx, %ecx

Lmld_rej_uniform_eta2_avx2_asm_loop:
        cmpl $0xf8, %eax
        ja Lmld_rej_uniform_eta2_avx2_asm_scalar
        cmpl $0x78, %ecx
        ja Lmld_rej_uniform_eta2_avx2_asm_scalar
        vpmovzxbw (%rsi,%rcx), %ymm0
        vpsllw $0x4, %ymm0, %ymm1
        vpor %ymm1, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubb %ymm5, %ymm0, %ymm1
        vpsubb %ymm0, %ymm4, %ymm0
        vpmovmskb %ymm1, %r8d
        vextracti128 $0x0, %ymm0, %xmm8
        movzbl %r8b, %r10d
        vmovq (%rdx,%r10,8), %xmm9
        vpshufb %xmm9, %xmm8, %xmm9
        vpmovsxbd %xmm9, %ymm1
        vpmulhrsw %ymm6, %ymm1, %ymm2
        vpmullw %ymm7, %ymm2, %ymm2
        vpaddd %ymm2, %ymm1, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcntl %r10d, %r9d
        addl %r9d, %eax
        shrl $0x8, %r8d
        addl $0x4, %ecx
        cmpl $0xf8, %eax
        ja Lmld_rej_uniform_eta2_avx2_asm_scalar
        vpsrldq $0x8, %xmm8, %xmm8      # xmm8 = xmm8[8,9,10,11,12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero
        movzbl %r8b, %r10d
        vmovq (%rdx,%r10,8), %xmm9
        vpshufb %xmm9, %xmm8, %xmm9
        vpmovsxbd %xmm9, %ymm1
        vpmulhrsw %ymm6, %ymm1, %ymm2
        vpmullw %ymm7, %ymm2, %ymm2
        vpaddd %ymm2, %ymm1, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcntl %r10d, %r9d
        addl %r9d, %eax
        shrl $0x8, %r8d
        addl $0x4, %ecx
        cmpl $0xf8, %eax
        ja Lmld_rej_uniform_eta2_avx2_asm_scalar
        vextracti128 $0x1, %ymm0, %xmm8
        movzbl %r8b, %r10d
        vmovq (%rdx,%r10,8), %xmm9
        vpshufb %xmm9, %xmm8, %xmm9
        vpmovsxbd %xmm9, %ymm1
        vpmulhrsw %ymm6, %ymm1, %ymm2
        vpmullw %ymm7, %ymm2, %ymm2
        vpaddd %ymm2, %ymm1, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcntl %r10d, %r9d
        addl %r9d, %eax
        shrl $0x8, %r8d
        addl $0x4, %ecx
        cmpl $0xf8, %eax
        ja Lmld_rej_uniform_eta2_avx2_asm_scalar
        vpsrldq $0x8, %xmm8, %xmm8      # xmm8 = xmm8[8,9,10,11,12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero
        movzbl %r8b, %r10d
        vmovq (%rdx,%r10,8), %xmm9
        vpshufb %xmm9, %xmm8, %xmm9
        vpmovsxbd %xmm9, %ymm1
        vpmulhrsw %ymm6, %ymm1, %ymm2
        vpmullw %ymm7, %ymm2, %ymm2
        vpaddd %ymm2, %ymm1, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcntl %r10d, %r9d
        addl %r9d, %eax
        addl $0x4, %ecx
        jmp Lmld_rej_uniform_eta2_avx2_asm_loop

Lmld_rej_uniform_eta2_avx2_asm_scalar:
        cmpl $0x100, %eax            # imm = 0x100
        jae Lmld_rej_uniform_eta2_avx2_asm_done
        cmpl $0x88, %ecx
        jae Lmld_rej_uniform_eta2_avx2_asm_done
        movzbl (%rsi,%rcx), %r11d
        incl %ecx
        movl %r11d, %r10d
        andl $0xf, %r10d
        cmpl $0xf, %r10d
        jae Lmld_rej_uniform_eta2_avx2_asm_high_nibble
        movl %r10d, %r11d
        imull $0xcd, %r11d, %r11d
        shrl $0xa, %r11d
        imull $0x5, %r11d, %r11d
        subl %r11d, %r10d
        movl $0x2, %r11d
        subl %r10d, %r11d
        movl %r11d, (%rdi,%rax,4)
        incl %eax
        cmpl $0x100, %eax            # imm = 0x100
        jae Lmld_rej_uniform_eta2_avx2_asm_done

Lmld_rej_uniform_eta2_avx2_asm_high_nibble:
        movzbl -0x1(%rsi,%rcx), %r11d
        shrl $0x4, %r11d
        andl $0xf, %r11d
        cmpl $0xf, %r11d
        jae Lmld_rej_uniform_eta2_avx2_asm_scalar
        movl %r11d, %r10d
        imull $0xcd, %r10d, %r10d
        shrl $0xa, %r10d
        imull $0x5, %r10d, %r10d
        subl %r10d, %r11d
        movl $0x2, %r10d
        subl %r11d, %r10d
        movl %r10d, (%rdi,%rax,4)
        incl %eax
        jmp Lmld_rej_uniform_eta2_avx2_asm_scalar

Lmld_rej_uniform_eta2_avx2_asm_done:
        retq
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_rej_uniform_eta2_avx2_asm)


#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
