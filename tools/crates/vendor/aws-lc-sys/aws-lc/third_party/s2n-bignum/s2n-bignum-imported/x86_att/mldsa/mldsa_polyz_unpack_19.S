// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Unpack polynomial z with 20-bit packed coefficients (GAMMA1 = 2^19)
//
// Reads 640 bytes of packed input (256 coefficients, 20 bits each) and writes
// 256 signed 32-bit output coefficients.  Each packed field x in [0, 2^20-1]
// is mapped to the signed value GAMMA1 - x in [-(2^19-1), 2^19].
//
// extern void mldsa_polyz_unpack_19_x86(int32_t r[static 256],
//                                   const uint8_t b[static 640]);
//
// Standard x86-64 ABI: RDI = r, RSI = b
// Microsoft x64 ABI:   RCX = r, RDX = b
// ----------------------------------------------------------------------------

// This implementation is derived from the public domain AVX2 Dilithium
// implementation (pq-crystals/dilithium) via the mldsa-native project.

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_polyz_unpack_19_x86)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_polyz_unpack_19_x86)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_polyz_unpack_19_x86)
        .text

// Unpack one AVX2 block of 8 coefficients.
//   src = byte offset of the 20-byte packed input chunk in a (%rsi)
//   dst = byte offset of the 32-byte output chunk in r (%rdi)
.macro zunpack19_block src, dst
        vmovdqu \src(%rsi), %xmm0
        vmovdqu \src + 4(%rsi), %xmm5
        vinserti128 $1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand   %ymm3, %ymm0, %ymm0
        vpsubd  %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, \dst(%rdi)
.endm

S2N_BN_SYMBOL(mldsa_polyz_unpack_19_x86):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_PUSH(%rdi)
        CFI_PUSH(%rsi)
        movq    %rcx, %rdi
        movq    %rdx, %rsi
#endif

// Set up constants:
//   %ymm1 = VPSHUFB byte-gather pattern (selects each 20-bit field's bytes)
//   %ymm2 = per-lane VPSRLVD shift amounts (0 and 4 alternating)
//   %ymm3 = 20-bit field mask (0xfffff broadcast)
//   %ymm4 = GAMMA1 = 2^19 (0x80000 broadcast)
        movabsq $0xFF040302FF020100, %rax
        vmovq   %rax, %xmm1
        movabsq $0xFF090807FF070605, %rax
        vpinsrq $1, %rax, %xmm1, %xmm1
        movabsq $0xFF1A1918FF181716, %rax
        vmovq   %rax, %xmm5
        movabsq $0xFF1F1E1DFF1D1C1B, %rax
        vpinsrq $1, %rax, %xmm5, %xmm5
        vinserti128 $1, %xmm5, %ymm1, %ymm1
        movabsq $0x400000000, %rax
        vmovq   %rax, %xmm2
        movabsq $0x400000000, %rax
        vpinsrq $1, %rax, %xmm2, %xmm2
        vinserti128 $1, %xmm2, %ymm2, %ymm2
        movl    $0xfffff, %eax
        vmovd   %eax, %xmm3
        vpbroadcastd %xmm3, %ymm3
        movl    $0x80000, %eax
        vmovd   %eax, %xmm4
        vpbroadcastd %xmm4, %ymm4

// Process all 256 coefficients (32 blocks of 8 int32 each)
        zunpack19_block 0, 0
        zunpack19_block 20, 32
        zunpack19_block 40, 64
        zunpack19_block 60, 96
        zunpack19_block 80, 128
        zunpack19_block 100, 160
        zunpack19_block 120, 192
        zunpack19_block 140, 224
        zunpack19_block 160, 256
        zunpack19_block 180, 288
        zunpack19_block 200, 320
        zunpack19_block 220, 352
        zunpack19_block 240, 384
        zunpack19_block 260, 416
        zunpack19_block 280, 448
        zunpack19_block 300, 480
        zunpack19_block 320, 512
        zunpack19_block 340, 544
        zunpack19_block 360, 576
        zunpack19_block 380, 608
        zunpack19_block 400, 640
        zunpack19_block 420, 672
        zunpack19_block 440, 704
        zunpack19_block 460, 736
        zunpack19_block 480, 768
        zunpack19_block 500, 800
        zunpack19_block 520, 832
        zunpack19_block 540, 864
        zunpack19_block 560, 896
        zunpack19_block 580, 928
        zunpack19_block 600, 960
        zunpack19_block 620, 992

#if WINDOWS_ABI
        CFI_POP(%rsi)
        CFI_POP(%rdi)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_polyz_unpack_19_x86)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
