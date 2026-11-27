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
  Name: polyz_unpack_19_avx2_asm
  Description: x86_64 AVX2 unpacking of 19-bit packed coefficients.
    Unpack polynomial z with 20-bit packed coefficients (GAMMA1 = 2^19),
    mapping packed [0, 2^20-1] to signed [-(2^19-1), 2^19] via GAMMA1 - x.
  Signature: void mld_polyz_unpack_19_avx2_asm(int32_t *r, const uint8_t *a)
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
      size_bytes: 640
      permissions: read-only
      c_parameter: const uint8_t *a
      description: Packed input bytes
*/

#include "_internal_s2n_bignum_x86_att.h"



/*
 * WARNING: This file is auto-derived from the mldsa-native source file
 *   dev/x86_64/src/mldsa_polyz_unpack_19_avx2_asm.S using scripts/simpasm. Do not modify it directly.
 */

.text
.balign 4
        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_polyz_unpack_19_avx2_asm)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_polyz_unpack_19_avx2_asm)
S2N_BN_SYMBOL(mldsa_polyz_unpack_19_avx2_asm):

        .cfi_startproc
        movabsq $-0xfbfcfd00fdff00, %rax # imm = 0xFF040302FF020100
        vmovq %rax, %xmm1
        movabsq $-0xf6f7f800f8f9fb, %rax # imm = 0xFF090807FF070605
        vpinsrq $0x1, %rax, %xmm1, %xmm1
        movabsq $-0xe5e6e700e7e8ea, %rax # imm = 0xFF1A1918FF181716
        vmovq %rax, %xmm5
        movabsq $-0xe0e1e200e2e3e5, %rax # imm = 0xFF1F1E1DFF1D1C1B
        vpinsrq $0x1, %rax, %xmm5, %xmm5
        vinserti128 $0x1, %xmm5, %ymm1, %ymm1
        movabsq $0x400000000, %rax      # imm = 0x400000000
        vmovq %rax, %xmm2
        movabsq $0x400000000, %rax      # imm = 0x400000000
        vpinsrq $0x1, %rax, %xmm2, %xmm2
        vinserti128 $0x1, %xmm2, %ymm2, %ymm2
        movl $0xfffff, %eax          # imm = 0xFFFFF
        vmovd %eax, %xmm3
        vpbroadcastd %xmm3, %ymm3
        movl $0x80000, %eax          # imm = 0x80000
        vmovd %eax, %xmm4
        vpbroadcastd %xmm4, %ymm4
        vmovdqu (%rsi), %xmm0
        vmovdqu 0x4(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, (%rdi)
        vmovdqu 0x14(%rsi), %xmm0
        vmovdqu 0x18(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x20(%rdi)
        vmovdqu 0x28(%rsi), %xmm0
        vmovdqu 0x2c(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x40(%rdi)
        vmovdqu 0x3c(%rsi), %xmm0
        vmovdqu 0x40(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x60(%rdi)
        vmovdqu 0x50(%rsi), %xmm0
        vmovdqu 0x54(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x80(%rdi)
        vmovdqu 0x64(%rsi), %xmm0
        vmovdqu 0x68(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0xa0(%rdi)
        vmovdqu 0x78(%rsi), %xmm0
        vmovdqu 0x7c(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0xc0(%rdi)
        vmovdqu 0x8c(%rsi), %xmm0
        vmovdqu 0x90(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0xe0(%rdi)
        vmovdqu 0xa0(%rsi), %xmm0
        vmovdqu 0xa4(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x100(%rdi)
        vmovdqu 0xb4(%rsi), %xmm0
        vmovdqu 0xb8(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x120(%rdi)
        vmovdqu 0xc8(%rsi), %xmm0
        vmovdqu 0xcc(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x140(%rdi)
        vmovdqu 0xdc(%rsi), %xmm0
        vmovdqu 0xe0(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x160(%rdi)
        vmovdqu 0xf0(%rsi), %xmm0
        vmovdqu 0xf4(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x180(%rdi)
        vmovdqu 0x104(%rsi), %xmm0
        vmovdqu 0x108(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x1a0(%rdi)
        vmovdqu 0x118(%rsi), %xmm0
        vmovdqu 0x11c(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x1c0(%rdi)
        vmovdqu 0x12c(%rsi), %xmm0
        vmovdqu 0x130(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x1e0(%rdi)
        vmovdqu 0x140(%rsi), %xmm0
        vmovdqu 0x144(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x200(%rdi)
        vmovdqu 0x154(%rsi), %xmm0
        vmovdqu 0x158(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x220(%rdi)
        vmovdqu 0x168(%rsi), %xmm0
        vmovdqu 0x16c(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x240(%rdi)
        vmovdqu 0x17c(%rsi), %xmm0
        vmovdqu 0x180(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x260(%rdi)
        vmovdqu 0x190(%rsi), %xmm0
        vmovdqu 0x194(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x280(%rdi)
        vmovdqu 0x1a4(%rsi), %xmm0
        vmovdqu 0x1a8(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x2a0(%rdi)
        vmovdqu 0x1b8(%rsi), %xmm0
        vmovdqu 0x1bc(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x2c0(%rdi)
        vmovdqu 0x1cc(%rsi), %xmm0
        vmovdqu 0x1d0(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x2e0(%rdi)
        vmovdqu 0x1e0(%rsi), %xmm0
        vmovdqu 0x1e4(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x300(%rdi)
        vmovdqu 0x1f4(%rsi), %xmm0
        vmovdqu 0x1f8(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x320(%rdi)
        vmovdqu 0x208(%rsi), %xmm0
        vmovdqu 0x20c(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x340(%rdi)
        vmovdqu 0x21c(%rsi), %xmm0
        vmovdqu 0x220(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x360(%rdi)
        vmovdqu 0x230(%rsi), %xmm0
        vmovdqu 0x234(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x380(%rdi)
        vmovdqu 0x244(%rsi), %xmm0
        vmovdqu 0x248(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x3a0(%rdi)
        vmovdqu 0x258(%rsi), %xmm0
        vmovdqu 0x25c(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x3c0(%rdi)
        vmovdqu 0x26c(%rsi), %xmm0
        vmovdqu 0x270(%rsi), %xmm5
        vinserti128 $0x1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand %ymm3, %ymm0, %ymm0
        vpsubd %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, 0x3e0(%rdi)
        retq
        .cfi_endproc

S2N_BN_SIZE_DIRECTIVE(mldsa_polyz_unpack_19_avx2_asm)



#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
