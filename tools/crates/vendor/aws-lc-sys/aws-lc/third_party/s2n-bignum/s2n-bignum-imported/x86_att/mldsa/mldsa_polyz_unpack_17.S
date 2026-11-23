// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Unpacking of 17-bit packed z polynomial coefficients for ML-DSA
// Output r[256] (signed 32-bit words), input b[576] (packed bytes)
//
// Unpacks polynomial z with 18-bit packed coefficients (GAMMA1 = 2^17),
// mapping packed [0, 2^18-1] to signed [-(2^17-1), 2^17] via GAMMA1 - x.
//
// extern void mldsa_polyz_unpack_17_x86(int32_t r[static 256], const uint8_t b[static 576]);
//
// Standard x86-64 ABI: RDI = r, RSI = b
// Microsoft x64 ABI:   RCX = r, RDX = b
// ----------------------------------------------------------------------------

// This implementation is derived from the public domain AVX2 Dilithium
// implementation (pq-crystals/dilithium) via the mldsa-native project.

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_polyz_unpack_17_x86)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_polyz_unpack_17_x86)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_polyz_unpack_17_x86)
        .text

// Unpack one block of 8 coefficients: gather two overlapping 128-bit loads
// from the packed input, byte-shuffle into 32-bit lanes, variable-shift and
// mask off the 18-bit fields, then form GAMMA1 - x and store the 256-bit
// result.
.macro zunpack17_block in_off, out_off
        vmovdqu \in_off(%rsi), %xmm0
        vmovdqu \in_off + 2(%rsi), %xmm5
        vinserti128 $1, %xmm5, %ymm0, %ymm0
        vpshufb %ymm1, %ymm0, %ymm0
        vpsrlvd %ymm2, %ymm0, %ymm0
        vpand   %ymm3, %ymm0, %ymm0
        vpsubd  %ymm0, %ymm4, %ymm0
        vmovdqa %ymm0, \out_off(%rdi)
.endm

S2N_BN_SYMBOL(mldsa_polyz_unpack_17_x86):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_PUSH(%rdi)
        CFI_PUSH(%rsi)
        movq    %rcx, %rdi
        movq    %rdx, %rsi
#endif

// Set up the shuffle table (%ymm1), the per-lane shift counts (%ymm2), the
// 18-bit field mask (%ymm3), and the GAMMA1 = 2^17 broadcast (%ymm4).
        movabsq $0xff040302ff020100, %rax
        vmovq   %rax, %xmm1
        movabsq $0xff080706ff060504, %rax
        vpinsrq $1, %rax, %xmm1, %xmm1
        movabsq $0xff1b1a19ff191817, %rax
        vmovq   %rax, %xmm5
        movabsq $0xff1f1e1dff1d1c1b, %rax
        vpinsrq $1, %rax, %xmm5, %xmm5
        vinserti128 $1, %xmm5, %ymm1, %ymm1
        movabsq $0x200000000, %rax
        vmovq   %rax, %xmm2
        movabsq $0x600000004, %rax
        vpinsrq $1, %rax, %xmm2, %xmm2
        vinserti128 $1, %xmm2, %ymm2, %ymm2
        movl    $0x3ffff, %eax
        vmovd   %eax, %xmm3
        vpbroadcastd %xmm3, %ymm3
        movl    $0x20000, %eax
        vmovd   %eax, %xmm4
        vpbroadcastd %xmm4, %ymm4

// Process all 256 coefficients (32 blocks of 8 int32 each). Each input block
// is 18 bytes; each output block is 32 bytes.
        zunpack17_block 0, 0
        zunpack17_block 18, 32
        zunpack17_block 36, 64
        zunpack17_block 54, 96
        zunpack17_block 72, 128
        zunpack17_block 90, 160
        zunpack17_block 108, 192
        zunpack17_block 126, 224
        zunpack17_block 144, 256
        zunpack17_block 162, 288
        zunpack17_block 180, 320
        zunpack17_block 198, 352
        zunpack17_block 216, 384
        zunpack17_block 234, 416
        zunpack17_block 252, 448
        zunpack17_block 270, 480
        zunpack17_block 288, 512
        zunpack17_block 306, 544
        zunpack17_block 324, 576
        zunpack17_block 342, 608
        zunpack17_block 360, 640
        zunpack17_block 378, 672
        zunpack17_block 396, 704
        zunpack17_block 414, 736
        zunpack17_block 432, 768
        zunpack17_block 450, 800
        zunpack17_block 468, 832
        zunpack17_block 486, 864
        zunpack17_block 504, 896
        zunpack17_block 522, 928
        zunpack17_block 540, 960
        zunpack17_block 558, 992

#if WINDOWS_ABI
        CFI_POP(%rsi)
        CFI_POP(%rdi)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_polyz_unpack_17_x86)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
