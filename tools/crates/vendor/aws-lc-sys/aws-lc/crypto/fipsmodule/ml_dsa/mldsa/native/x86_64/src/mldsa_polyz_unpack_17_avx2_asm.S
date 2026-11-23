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
  Name: polyz_unpack_17_avx2_asm
  Description: x86_64 AVX2 unpacking of 17-bit packed coefficients.
    Unpack polynomial z with 18-bit packed coefficients (GAMMA1 = 2^17),
    mapping packed [0, 2^18-1] to signed [-(2^17-1), 2^17] via GAMMA1 - x.
  Signature: void mld_polyz_unpack_17_avx2_asm(int32_t *r, const uint8_t *a)
  ABI:
    Architecture: x86_64
    CallingConvention: SysV
    Features: [AVX2]
    rdi:
      type: buffer
      size_bytes: 1024
      permissions: write-only
      c_parameter: int32_t *r
      description: Output polynomial (256 x int32_t)
    rsi:
      type: buffer
      size_bytes: 576
      permissions: read-only
      c_parameter: const uint8_t *a
      description: Packed input bytes
*/

#include "_internal_s2n_bignum_x86_att.h"



/*
 * WARNING: This file is auto-derived from the mldsa-native source file
 *   dev/x86_64/src/mldsa_polyz_unpack_17_avx2_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_polyz_unpack_17_avx2_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_polyz_unpack_17_avx2_asm)
S2N_BN_SYMBOL(mldsa_polyz_unpack_17_avx2_asm):

        .cfi_startproc
        movabsq $-0xfbfcfd00fdff00, %rax # imm = 0xFF040302FF020100
        vmovq %rax, %xmm1
        movabsq $-0xf7f8f900f9fafc, %rax # imm = 0xFF080706FF060504
        vpinsrq $0x1, %rax, %xmm1, %xmm1
        movabsq $-0xe4e5e600e6e7e9, %rax # imm = 0xFF1B1A19FF191817
        vmovq %rax, %xmm5
        movabsq $-0xe0e1e200e2e3e5, %rax # imm = 0xFF1F1E1DFF1D1C1B
        vpinsrq $0x1, %rax, %xmm5, %xmm5
        vinserti128 $0x1, %xmm5, %ymm1, %ymm1
        movabsq $0x200000000, %rax      # imm = 0x200000000
        vmovq %rax, %xmm2
        movabsq $0x600000004, %rax      # imm = 0x600000004
        vpinsrq $0x1, %rax, %xmm2, %xmm2
        vinserti128 $0x1, %xmm2, %ymm2, %ymm2
        movl $0x3ffff, %eax          # imm = 0x3FFFF
        vmovd %eax, %xmm3
        vpbroadcastd %xmm3, %ymm3
        movl $0x20000, %eax          # imm = 0x20000
        vmovd %eax, %xmm4
        vpbroadcastd %xmm4, %ymm4
        vmovdqu (%rsi), %xmm0
        vmovdqu 0x2(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, (%rdi)
        vmovdqu 0x12(%rsi), %xmm0
        vmovdqu 0x14(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x20(%rdi)
        vmovdqu 0x24(%rsi), %xmm0
        vmovdqu 0x26(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x40(%rdi)
        vmovdqu 0x36(%rsi), %xmm0
        vmovdqu 0x38(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x60(%rdi)
        vmovdqu 0x48(%rsi), %xmm0
        vmovdqu 0x4a(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x80(%rdi)
        vmovdqu 0x5a(%rsi), %xmm0
        vmovdqu 0x5c(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0xa0(%rdi)
        vmovdqu 0x6c(%rsi), %xmm0
        vmovdqu 0x6e(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0xc0(%rdi)
        vmovdqu 0x7e(%rsi), %xmm0
        vmovdqu 0x80(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0xe0(%rdi)
        vmovdqu 0x90(%rsi), %xmm0
        vmovdqu 0x92(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x100(%rdi)
        vmovdqu 0xa2(%rsi), %xmm0
        vmovdqu 0xa4(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x120(%rdi)
        vmovdqu 0xb4(%rsi), %xmm0
        vmovdqu 0xb6(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x140(%rdi)
        vmovdqu 0xc6(%rsi), %xmm0
        vmovdqu 0xc8(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x160(%rdi)
        vmovdqu 0xd8(%rsi), %xmm0
        vmovdqu 0xda(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x180(%rdi)
        vmovdqu 0xea(%rsi), %xmm0
        vmovdqu 0xec(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x1a0(%rdi)
        vmovdqu 0xfc(%rsi), %xmm0
        vmovdqu 0xfe(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x1c0(%rdi)
        vmovdqu 0x10e(%rsi), %xmm0
        vmovdqu 0x110(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x1e0(%rdi)
        vmovdqu 0x120(%rsi), %xmm0
        vmovdqu 0x122(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x200(%rdi)
        vmovdqu 0x132(%rsi), %xmm0
        vmovdqu 0x134(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x220(%rdi)
        vmovdqu 0x144(%rsi), %xmm0
        vmovdqu 0x146(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x240(%rdi)
        vmovdqu 0x156(%rsi), %xmm0
        vmovdqu 0x158(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x260(%rdi)
        vmovdqu 0x168(%rsi), %xmm0
        vmovdqu 0x16a(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x280(%rdi)
        vmovdqu 0x17a(%rsi), %xmm0
        vmovdqu 0x17c(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x2a0(%rdi)
        vmovdqu 0x18c(%rsi), %xmm0
        vmovdqu 0x18e(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x2c0(%rdi)
        vmovdqu 0x19e(%rsi), %xmm0
        vmovdqu 0x1a0(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x2e0(%rdi)
        vmovdqu 0x1b0(%rsi), %xmm0
        vmovdqu 0x1b2(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x300(%rdi)
        vmovdqu 0x1c2(%rsi), %xmm0
        vmovdqu 0x1c4(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x320(%rdi)
        vmovdqu 0x1d4(%rsi), %xmm0
        vmovdqu 0x1d6(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x340(%rdi)
        vmovdqu 0x1e6(%rsi), %xmm0
        vmovdqu 0x1e8(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x360(%rdi)
        vmovdqu 0x1f8(%rsi), %xmm0
        vmovdqu 0x1fa(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x380(%rdi)
        vmovdqu 0x20a(%rsi), %xmm0
        vmovdqu 0x20c(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x3a0(%rdi)
        vmovdqu 0x21c(%rsi), %xmm0
        vmovdqu 0x21e(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x3c0(%rdi)
        vmovdqu 0x22e(%rsi), %xmm0
        vmovdqu 0x230(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x3e0(%rdi)
        retq
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_polyz_unpack_17_avx2_asm)



#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
