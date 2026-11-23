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
 *
 * The algorithm for Decompose(r) (more specifically the handling for the
 * wrap-around cases) is modified. See the AVX2 intrinsics version
 * (poly_decompose_32_avx2.c, predecessor of this file) for a more detailed
 * comparison.
 */


/*yaml
  Name: poly_decompose_32_avx2_asm
  Description: x86_64 AVX2 coefficient decomposition (alpha = 2*(Q-1)/32).
    For all coefficients c of the input polynomial, compute high and low bits
    c0, c1 such that c = c1*ALPHA + c0 with -ALPHA/2 < c0 <= ALPHA/2, except if
    c1 = (Q-1)/ALPHA where we set c1 = 0 and -ALPHA/2 <= c0 = c - Q < 0.
    Assumes coefficients to be standard (unsigned canonical) representatives,
    i.e. 0 <= c < Q. For ML-DSA-65 / ML-DSA-87 (gamma2 = (Q-1)/32,
    alpha = 2*gamma2).
  Signature: void mld_poly_decompose_32_avx2_asm(int32_t *a1, int32_t *a0)
  ABI:
    Architecture: x86_64
    CallingConvention: SysV
    Features: [AVX2]
    rdi:
      type: buffer
      size_bytes: 1024
      permissions: write-only
      c_parameter: int32_t *a1
      description: Output high-part polynomial (256 x int32_t)
    rsi:
      type: buffer
      size_bytes: 1024
      permissions: read/write
      c_parameter: int32_t *a0
      description: Input polynomial / output low-part (256 x int32_t)
*/

#include "_internal_s2n_bignum_x86_att.h"



/*
 * WARNING: This file is auto-derived from the mldsa-native source file
 *   dev/x86_64/src/mldsa_poly_decompose_32_avx2_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_poly_decompose_32_avx2_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_poly_decompose_32_avx2_asm)
S2N_BN_SYMBOL(mldsa_poly_decompose_32_avx2_asm):

        .cfi_startproc
        movl $0x7f, %eax
        vmovd %eax, %xmm10
        vpbroadcastd %xmm10, %ymm10
        movl $0x401, %eax            # imm = 0x401
        vmovd %eax, %xmm11
        vpbroadcastd %xmm11, %ymm11
        movl $0x200, %eax            # imm = 0x200
        vmovd %eax, %xmm12
        vpbroadcastd %xmm12, %ymm12
        movl $0x7be100, %eax         # imm = 0x7BE100
        vmovd %eax, %xmm13
        vpbroadcastd %xmm13, %ymm13
        movl $0x7fe00, %eax          # imm = 0x7FE00
        vmovd %eax, %xmm14
        vpbroadcastd %xmm14, %ymm14
        vmovdqa (%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, (%rdi)
        vmovdqa %ymm2, (%rsi)
        vmovdqa 0x20(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x20(%rdi)
        vmovdqa %ymm2, 0x20(%rsi)
        vmovdqa 0x40(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x40(%rdi)
        vmovdqa %ymm2, 0x40(%rsi)
        vmovdqa 0x60(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x60(%rdi)
        vmovdqa %ymm2, 0x60(%rsi)
        vmovdqa 0x80(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x80(%rdi)
        vmovdqa %ymm2, 0x80(%rsi)
        vmovdqa 0xa0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0xa0(%rdi)
        vmovdqa %ymm2, 0xa0(%rsi)
        vmovdqa 0xc0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0xc0(%rdi)
        vmovdqa %ymm2, 0xc0(%rsi)
        vmovdqa 0xe0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0xe0(%rdi)
        vmovdqa %ymm2, 0xe0(%rsi)
        vmovdqa 0x100(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x100(%rdi)
        vmovdqa %ymm2, 0x100(%rsi)
        vmovdqa 0x120(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x120(%rdi)
        vmovdqa %ymm2, 0x120(%rsi)
        vmovdqa 0x140(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x140(%rdi)
        vmovdqa %ymm2, 0x140(%rsi)
        vmovdqa 0x160(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x160(%rdi)
        vmovdqa %ymm2, 0x160(%rsi)
        vmovdqa 0x180(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x180(%rdi)
        vmovdqa %ymm2, 0x180(%rsi)
        vmovdqa 0x1a0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x1a0(%rdi)
        vmovdqa %ymm2, 0x1a0(%rsi)
        vmovdqa 0x1c0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x1c0(%rdi)
        vmovdqa %ymm2, 0x1c0(%rsi)
        vmovdqa 0x1e0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x1e0(%rdi)
        vmovdqa %ymm2, 0x1e0(%rsi)
        vmovdqa 0x200(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x200(%rdi)
        vmovdqa %ymm2, 0x200(%rsi)
        vmovdqa 0x220(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x220(%rdi)
        vmovdqa %ymm2, 0x220(%rsi)
        vmovdqa 0x240(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x240(%rdi)
        vmovdqa %ymm2, 0x240(%rsi)
        vmovdqa 0x260(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x260(%rdi)
        vmovdqa %ymm2, 0x260(%rsi)
        vmovdqa 0x280(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x280(%rdi)
        vmovdqa %ymm2, 0x280(%rsi)
        vmovdqa 0x2a0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x2a0(%rdi)
        vmovdqa %ymm2, 0x2a0(%rsi)
        vmovdqa 0x2c0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x2c0(%rdi)
        vmovdqa %ymm2, 0x2c0(%rsi)
        vmovdqa 0x2e0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x2e0(%rdi)
        vmovdqa %ymm2, 0x2e0(%rsi)
        vmovdqa 0x300(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x300(%rdi)
        vmovdqa %ymm2, 0x300(%rsi)
        vmovdqa 0x320(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x320(%rdi)
        vmovdqa %ymm2, 0x320(%rsi)
        vmovdqa 0x340(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x340(%rdi)
        vmovdqa %ymm2, 0x340(%rsi)
        vmovdqa 0x360(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x360(%rdi)
        vmovdqa %ymm2, 0x360(%rsi)
        vmovdqa 0x380(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x380(%rdi)
        vmovdqa %ymm2, 0x380(%rsi)
        vmovdqa 0x3a0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x3a0(%rdi)
        vmovdqa %ymm2, 0x3a0(%rsi)
        vmovdqa 0x3c0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x3c0(%rdi)
        vmovdqa %ymm2, 0x3c0(%rsi)
        vmovdqa 0x3e0(%rsi), %ymm0
        vpaddd %ymm10, %ymm0, %ymm1
        vpsrld $0x7, %ymm1, %ymm1
        vpmulhuw %ymm11, %ymm1, %ymm1
        vpmulhrsw %ymm12, %ymm1, %ymm1
        vpcmpgtd %ymm13, %ymm0, %ymm3
        vpmulld %ymm14, %ymm1, %ymm2
        vpsubd %ymm2, %ymm0, %ymm2
        vpandn %ymm1, %ymm3, %ymm1
        vpaddd %ymm3, %ymm2, %ymm2
        vmovdqa %ymm1, 0x3e0(%rdi)
        vmovdqa %ymm2, 0x3e0(%rsi)
        retq
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_poly_decompose_32_avx2_asm)



#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
