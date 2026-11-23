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
  Name: poly_chknorm_avx2_asm
  Description: x86_64 AVX2 infinity-norm bound check on polynomial coefficients.
    Check the infinity norm of the polynomial against the given bound B.
    Returns 0 if the norm is strictly smaller than B; otherwise returns 1
    (i.e. returns 1 if any |coefficient| >= B, 0 otherwise).
  Signature: int mld_poly_chknorm_avx2_asm(const int32_t *a, int32_t B)
  ABI:
    Architecture: x86_64
    CallingConvention: SysV
    Features: [AVX2]
    rdi:
      type: buffer
      size_bytes: 1024
      permissions: read-only
      c_parameter: const int32_t *a
      description: Input polynomial (256 x int32_t)
    rsi:
      type: scalar
      c_parameter: int32_t B
      description: Norm bound (must be non-negative)
      test_with: 131072  # representative non-negative bound (1 << 17)
*/

#include "_internal_s2n_bignum_x86_att.h"



/*
 * WARNING: This file is auto-derived from the mldsa-native source file
 *   dev/x86_64/src/mldsa_poly_chknorm_avx2_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_poly_chknorm_avx2_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_poly_chknorm_avx2_asm)
S2N_BN_SYMBOL(mldsa_poly_chknorm_avx2_asm):

        .cfi_startproc
        subl $0x1, %esi
        vpxor %xmm1, %xmm1, %xmm1
        vmovd %esi, %xmm2
        vpbroadcastd %xmm2, %ymm2
        vpabsd (%rdi), %ymm0
        vpcmpgtd %ymm2, %ymm0, %ymm0
        vpor %ymm0, %ymm1, %ymm1
        vpabsd 0x20(%rdi), %ymm3
        vpcmpgtd %ymm2, %ymm3, %ymm3
        vpor %ymm3, %ymm1, %ymm1
        vpabsd 0x40(%rdi), %ymm4
        vpcmpgtd %ymm2, %ymm4, %ymm4
        vpor %ymm4, %ymm1, %ymm1
        vpabsd 0x60(%rdi), %ymm5
        vpcmpgtd %ymm2, %ymm5, %ymm5
        vpor %ymm5, %ymm1, %ymm1
        vpabsd 0x80(%rdi), %ymm0
        vpcmpgtd %ymm2, %ymm0, %ymm0
        vpor %ymm0, %ymm1, %ymm1
        vpabsd 0xa0(%rdi), %ymm3
        vpcmpgtd %ymm2, %ymm3, %ymm3
        vpor %ymm3, %ymm1, %ymm1
        vpabsd 0xc0(%rdi), %ymm4
        vpcmpgtd %ymm2, %ymm4, %ymm4
        vpor %ymm4, %ymm1, %ymm1
        vpabsd 0xe0(%rdi), %ymm5
        vpcmpgtd %ymm2, %ymm5, %ymm5
        vpor %ymm5, %ymm1, %ymm1
        vpabsd 0x100(%rdi), %ymm0
        vpcmpgtd %ymm2, %ymm0, %ymm0
        vpor %ymm0, %ymm1, %ymm1
        vpabsd 0x120(%rdi), %ymm3
        vpcmpgtd %ymm2, %ymm3, %ymm3
        vpor %ymm3, %ymm1, %ymm1
        vpabsd 0x140(%rdi), %ymm4
        vpcmpgtd %ymm2, %ymm4, %ymm4
        vpor %ymm4, %ymm1, %ymm1
        vpabsd 0x160(%rdi), %ymm5
        vpcmpgtd %ymm2, %ymm5, %ymm5
        vpor %ymm5, %ymm1, %ymm1
        vpabsd 0x180(%rdi), %ymm0
        vpcmpgtd %ymm2, %ymm0, %ymm0
        vpor %ymm0, %ymm1, %ymm1
        vpabsd 0x1a0(%rdi), %ymm3
        vpcmpgtd %ymm2, %ymm3, %ymm3
        vpor %ymm3, %ymm1, %ymm1
        vpabsd 0x1c0(%rdi), %ymm4
        vpcmpgtd %ymm2, %ymm4, %ymm4
        vpor %ymm4, %ymm1, %ymm1
        vpabsd 0x1e0(%rdi), %ymm5
        vpcmpgtd %ymm2, %ymm5, %ymm5
        vpor %ymm5, %ymm1, %ymm1
        vpabsd 0x200(%rdi), %ymm0
        vpcmpgtd %ymm2, %ymm0, %ymm0
        vpor %ymm0, %ymm1, %ymm1
        vpabsd 0x220(%rdi), %ymm3
        vpcmpgtd %ymm2, %ymm3, %ymm3
        vpor %ymm3, %ymm1, %ymm1
        vpabsd 0x240(%rdi), %ymm4
        vpcmpgtd %ymm2, %ymm4, %ymm4
        vpor %ymm4, %ymm1, %ymm1
        vpabsd 0x260(%rdi), %ymm5
        vpcmpgtd %ymm2, %ymm5, %ymm5
        vpor %ymm5, %ymm1, %ymm1
        vpabsd 0x280(%rdi), %ymm0
        vpcmpgtd %ymm2, %ymm0, %ymm0
        vpor %ymm0, %ymm1, %ymm1
        vpabsd 0x2a0(%rdi), %ymm3
        vpcmpgtd %ymm2, %ymm3, %ymm3
        vpor %ymm3, %ymm1, %ymm1
        vpabsd 0x2c0(%rdi), %ymm4
        vpcmpgtd %ymm2, %ymm4, %ymm4
        vpor %ymm4, %ymm1, %ymm1
        vpabsd 0x2e0(%rdi), %ymm5
        vpcmpgtd %ymm2, %ymm5, %ymm5
        vpor %ymm5, %ymm1, %ymm1
        vpabsd 0x300(%rdi), %ymm0
        vpcmpgtd %ymm2, %ymm0, %ymm0
        vpor %ymm0, %ymm1, %ymm1
        vpabsd 0x320(%rdi), %ymm3
        vpcmpgtd %ymm2, %ymm3, %ymm3
        vpor %ymm3, %ymm1, %ymm1
        vpabsd 0x340(%rdi), %ymm4
        vpcmpgtd %ymm2, %ymm4, %ymm4
        vpor %ymm4, %ymm1, %ymm1
        vpabsd 0x360(%rdi), %ymm5
        vpcmpgtd %ymm2, %ymm5, %ymm5
        vpor %ymm5, %ymm1, %ymm1
        vpabsd 0x380(%rdi), %ymm0
        vpcmpgtd %ymm2, %ymm0, %ymm0
        vpor %ymm0, %ymm1, %ymm1
        vpabsd 0x3a0(%rdi), %ymm3
        vpcmpgtd %ymm2, %ymm3, %ymm3
        vpor %ymm3, %ymm1, %ymm1
        vpabsd 0x3c0(%rdi), %ymm4
        vpcmpgtd %ymm2, %ymm4, %ymm4
        vpor %ymm4, %ymm1, %ymm1
        vpabsd 0x3e0(%rdi), %ymm5
        vpcmpgtd %ymm2, %ymm5, %ymm5
        vpor %ymm5, %ymm1, %ymm1
        xorl %eax, %eax
        vptest %ymm1, %ymm1
        setne %al
        retq
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_poly_chknorm_avx2_asm)



#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
