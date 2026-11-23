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
  Name: poly_use_hint_32_avx2_asm
  Description: x86_64 AVX2 hint application (alpha = (Q-1)/32).
    Use the hint polynomial h to correct the high bits of the polynomial a,
    in place. Variant for parameter sets ML-DSA-65 and ML-DSA-87
    (GAMMA2 = (Q-1)/32).
  Signature: void mld_poly_use_hint_32_avx2_asm(int32_t *a, const int32_t *h)
  ABI:
    Architecture: x86_64
    CallingConvention: SysV
    Features: [AVX2]
    rdi:
      type: buffer
      size_bytes: 1024
      permissions: read/write
      c_parameter: int32_t *a
      description: Input/output polynomial (256 x int32_t)
    rsi:
      type: buffer
      size_bytes: 1024
      permissions: read-only
      c_parameter: const int32_t *h
      description: Hint polynomial (256 x int32_t)
*/

#include "_internal_s2n_bignum_x86_att.h"



/*
 * WARNING: This file is auto-derived from the mldsa-native source file
 *   dev/x86_64/src/mldsa_poly_use_hint_32_avx2_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_poly_use_hint_32_avx2_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_poly_use_hint_32_avx2_asm)
S2N_BN_SYMBOL(mldsa_poly_use_hint_32_avx2_asm):

        .cfi_startproc
        movl $0x7f, %ecx
        movl $0x401, %r8d            # imm = 0x401
        vmovd %r8d, %xmm8
        vpbroadcastd %xmm8, %ymm8
        xorl %eax, %eax
        vpxor %xmm6, %xmm6, %xmm6
        vmovd %ecx, %xmm5
        movl $0x7be100, %ecx         # imm = 0x7BE100
        movl $0x200, %r9d            # imm = 0x200
        vmovd %r9d, %xmm7
        vpbroadcastd %xmm7, %ymm7
        vmovd %ecx, %xmm4
        movl $0xf, %ecx
        vpbroadcastd %xmm5, %ymm5
        vmovd %ecx, %xmm3
        vpbroadcastd %xmm4, %ymm4
        vpbroadcastd %xmm3, %ymm3

Lmld_poly_use_hint_32_avx2_asm_loop:
        vmovdqa (%rdi), %ymm0
        vmovdqa (%rsi), %ymm2
        vpaddd %ymm0, %ymm5, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm8, %ymm1, %ymm1
        vpmulhrsw %ymm7, %ymm1, %ymm1
        vpcmpgtd %ymm4, %ymm0, %ymm11
        vpandn %ymm1, %ymm11, %ymm9
        vpslld $0xa, %ymm1, %ymm10
        vpsubd %ymm1, %ymm10, %ymm1
        vpslld $0x9, %ymm1, %ymm1
        vpsubd %ymm1, %ymm0, %ymm0
        vpaddd %ymm11, %ymm0, %ymm0
        vpcmpgtd %ymm6, %ymm0, %ymm0
        vpandn %ymm2, %ymm0, %ymm0
        vpslld $0x1, %ymm0, %ymm0
        vpsubd %ymm0, %ymm2, %ymm2
        vpaddd %ymm9, %ymm2, %ymm2
        vpand %ymm3, %ymm2, %ymm2
        vmovdqa %ymm2, (%rdi)
        addq $0x20, %rdi
        addq $0x20, %rsi
        addq $0x20, %rax
        cmpq $0x400, %rax            # imm = 0x400
        jne Lmld_poly_use_hint_32_avx2_asm_loop
        retq
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_poly_use_hint_32_avx2_asm)



#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
