// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// NTT unpack for ML-DSA
// Input a[256] (signed 32-bit words); output a[256] (signed 32-bit words)
//
// This function performs the unpacking transformation on NTT-domain polynomial
// coefficients, converting from a packed representation to a standard layout.
// The operation is performed in-place on a 256-element array of signed 32-bit
// integers. This is a supporting operation for ML-DSA signature operations.
//
// This implementation is derived from the public domain AVX2 Dilithium
// implementation from CRYSTALS-Dilithium optimized AVX2 implementation by
// Bai, Ducas, Kiltz, Lepoint, Lyubashevsky, Schwabe, Seiler, Stehlé
// (https://github.com/pq-crystals/dilithium/tree/master/avx2)
//
// extern void mldsa_nttunpack(int32_t a[static 256]);
//
// Standard x86-64 ABI: RDI = a
// Microsoft x64 ABI:   RCX = a
// ----------------------------------------------------------------------------

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_nttunpack)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_nttunpack)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_nttunpack)
        .text

#define a %rdi

.macro shuffle8 r0, r1, r2, r3
        vperm2i128      $0x20, \r1, \r0, \r2
        vperm2i128      $0x31, \r1, \r0, \r3
.endm

.macro shuffle4 r0, r1, r2, r3
        vpunpcklqdq     \r1, \r0, \r2
        vpunpckhqdq     \r1, \r0, \r3
.endm

.macro shuffle2 r0, r1, r2, r3
        vmovsldup       \r1, \r2
        vpblendd        $0xAA, \r2, \r0, \r2
        vpsrlq          $32, \r0, \r0
        vpblendd        $0xAA, \r1, \r0, \r3
.endm

.macro nttunpack_64_coefficients offset
        /* load */
        vmovdqa         (\offset+0)(%rdi), %ymm4
        vmovdqa         (\offset+32)(%rdi), %ymm5
        vmovdqa         (\offset+64)(%rdi), %ymm6
        vmovdqa         (\offset+96)(%rdi), %ymm7
        vmovdqa         (\offset+128)(%rdi), %ymm8
        vmovdqa         (\offset+160)(%rdi), %ymm9
        vmovdqa         (\offset+192)(%rdi), %ymm10
        vmovdqa         (\offset+224)(%rdi), %ymm11

        shuffle8        %ymm4,%ymm8,%ymm3,%ymm8
        shuffle8        %ymm5,%ymm9,%ymm4,%ymm9
        shuffle8        %ymm6,%ymm10,%ymm5,%ymm10
        shuffle8        %ymm7,%ymm11,%ymm6,%ymm11

        shuffle4        %ymm3,%ymm5,%ymm7,%ymm5
        shuffle4        %ymm8,%ymm10,%ymm3,%ymm10
        shuffle4        %ymm4,%ymm6,%ymm8,%ymm6
        shuffle4        %ymm9,%ymm11,%ymm4,%ymm11

        shuffle2        %ymm7,%ymm8,%ymm9,%ymm8
        shuffle2        %ymm5,%ymm6,%ymm7,%ymm6
        shuffle2        %ymm3,%ymm4,%ymm5,%ymm4
        shuffle2        %ymm10,%ymm11,%ymm3,%ymm11

        /* store */
        vmovdqa         %ymm9, (\offset+0)(%rdi)
        vmovdqa         %ymm8, (\offset+32)(%rdi)
        vmovdqa         %ymm7, (\offset+64)(%rdi)
        vmovdqa         %ymm6, (\offset+96)(%rdi)
        vmovdqa         %ymm5, (\offset+128)(%rdi)
        vmovdqa         %ymm4, (\offset+160)(%rdi)
        vmovdqa         %ymm3, (\offset+192)(%rdi)
        vmovdqa         %ymm11, (\offset+224)(%rdi)
.endm

        .balign 4
S2N_BN_SYMBOL(mldsa_nttunpack):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        CFI_DEC_RSP(176)
        CFI_STACKSAVEU(%xmm6,0)
        CFI_STACKSAVEU(%xmm7,16)
        CFI_STACKSAVEU(%xmm8,32)
        CFI_STACKSAVEU(%xmm9,48)
        CFI_STACKSAVEU(%xmm10,64)
        CFI_STACKSAVEU(%xmm11,80)
        CFI_STACKSAVEU(%xmm12,96)
        CFI_STACKSAVEU(%xmm13,112)
        CFI_STACKSAVEU(%xmm14,128)
        CFI_STACKSAVEU(%xmm15,144)
        CFI_STACKSAVE(%rdi,160)
        movq    %rcx, %rdi
#endif

        nttunpack_64_coefficients 0
        nttunpack_64_coefficients 256
        nttunpack_64_coefficients 512
        nttunpack_64_coefficients 768

#if WINDOWS_ABI
        CFI_STACKLOADU(%xmm6,0)
        CFI_STACKLOADU(%xmm7,16)
        CFI_STACKLOADU(%xmm8,32)
        CFI_STACKLOADU(%xmm9,48)
        CFI_STACKLOADU(%xmm10,64)
        CFI_STACKLOADU(%xmm11,80)
        CFI_STACKLOADU(%xmm12,96)
        CFI_STACKLOADU(%xmm13,112)
        CFI_STACKLOADU(%xmm14,128)
        CFI_STACKLOADU(%xmm15,144)
        CFI_STACKLOAD(%rdi,160)
        CFI_INC_RSP(176)
#endif

        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_nttunpack)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
