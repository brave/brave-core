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
  Name: rej_uniform_avx2_asm
  Description: x86_64 AVX2 rejection sampling of uniform coefficients mod q.
    Extract 23-bit values from the input byte buffer and accept those that are
    < MLDSA_Q, writing accepted coefficients to the output buffer. Returns the
    number of valid coefficients written (at most 256).
  Signature: unsigned mld_rej_uniform_avx2_asm(int32_t *r, const uint8_t buf[840], const uint8_t table[256][8])
  ABI:
    Architecture: x86_64
    CallingConvention: SysV
    Features: [AVX2]
    rdi:
      type: buffer
      size_bytes: 1024
      permissions: write-only
      c_parameter: int32_t *r
      description: Output buffer (256 x int32_t)
    rsi:
      type: buffer
      size_bytes: 840
      permissions: read-only
      c_parameter: const uint8_t buf[840]
      description: Input buffer (MLD_POLY_UNIFORM_NBLOCKS * SHAKE128_RATE = 5 * 168)
    rdx:
      type: buffer
      size_bytes: 2048
      permissions: read-only
      c_parameter: const uint8_t table[256][8]
      description: Lookup table (256 x 8 x uint8_t)
*/

#include "_internal_s2n_bignum_x86_att.h"

/*
 * WARNING: This file is auto-derived from the mldsa-native source file
 *   dev/x86_64/src/mldsa_rej_uniform_avx2_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_rej_uniform_avx2_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_rej_uniform_avx2_asm)
S2N_BN_SYMBOL(mldsa_rej_uniform_avx2_asm):

        .cfi_startproc
        movabsq $-0xfafbfc00fdff00, %r10 # imm = 0xFF050403FF020100
        vmovq %r10, %xmm0
        movabsq $-0xf4f5f600f7f8fa, %r10 # imm = 0xFF0B0A09FF080706
        vpinsrq $0x1, %r10, %xmm0, %xmm0
        movabsq $-0xf6f7f800f9fafc, %r10 # imm = 0xFF090807FF060504
        vmovq %r10, %xmm3
        movabsq $-0xf0f1f200f3f4f6, %r10 # imm = 0xFF0F0E0DFF0C0B0A
        vpinsrq $0x1, %r10, %xmm3, %xmm3
        vinserti128 $0x1, %xmm3, %ymm0, %ymm0
        movl $0x7fffff, %r8d         # imm = 0x7FFFFF
        vmovd %r8d, %xmm1
        vpbroadcastd %xmm1, %ymm1
        movl $0x7fe001, %r8d         # imm = 0x7FE001
        vmovd %r8d, %xmm2
        vpbroadcastd %xmm2, %ymm2
        xorl %eax, %eax
        xorl %ecx, %ecx

Lmld_rej_uniform_avx2_asm_loop:
        cmpl $0xf8, %eax
        ja Lmld_rej_uniform_avx2_asm_scalar
        cmpl $0x328, %ecx            # imm = 0x328
        ja Lmld_rej_uniform_avx2_asm_scalar
        vmovdqu (%rsi,%rcx), %ymm3
        addl $0x18, %ecx
        vpermq $0x94, %ymm3, %ymm3     # ymm3 = ymm3[0,1,1,2]
        vpshufb %ymm0, %ymm3, %ymm3
        vpand %ymm1, %ymm3, %ymm3
        vpsubd %ymm2, %ymm3, %ymm4
        vmovmskps %ymm4, %r8d
        popcntl %r8d, %r9d
        vmovq (%rdx,%r8,8), %xmm4
        vpmovzxbd %xmm4, %ymm4    # ymm4 = xmm4[0],zero,zero,zero,xmm4[1],zero,zero,zero,xmm4[2],zero,zero,zero,xmm4[3],zero,zero,zero,xmm4[4],zero,zero,zero,xmm4[5],zero,zero,zero,xmm4[6],zero,zero,zero,xmm4[7],zero,zero,zero
        vpermd %ymm3, %ymm4, %ymm3
        vmovdqu %ymm3, (%rdi,%rax,4)
        addl %r9d, %eax
        jmp Lmld_rej_uniform_avx2_asm_loop

Lmld_rej_uniform_avx2_asm_scalar:
        cmpl $0x100, %eax            # imm = 0x100
        jae Lmld_rej_uniform_avx2_asm_done
        cmpl $0x345, %ecx            # imm = 0x345
        ja Lmld_rej_uniform_avx2_asm_done
        movzwl (%rsi,%rcx), %r8d
        movzbl 0x2(%rsi,%rcx), %r9d
        shll $0x10, %r9d
        orl %r9d, %r8d
        andl $0x7fffff, %r8d         # imm = 0x7FFFFF
        addl $0x3, %ecx
        cmpl $0x7fe001, %r8d         # imm = 0x7FE001
        jae Lmld_rej_uniform_avx2_asm_scalar
        movl %r8d, (%rdi,%rax,4)
        addl $0x1, %eax
        jmp Lmld_rej_uniform_avx2_asm_scalar

Lmld_rej_uniform_avx2_asm_done:
        retq
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_rej_uniform_avx2_asm)


#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
