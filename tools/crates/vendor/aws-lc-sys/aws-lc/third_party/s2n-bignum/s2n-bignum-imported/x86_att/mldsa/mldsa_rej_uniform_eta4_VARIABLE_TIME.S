// Copyright (c) The mldsa-native project authors
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Rejection sampling of eta=4 secret coefficients for ML-DSA (x86_64 AVX2)
//
// Extracts 4-bit nibbles from the input byte buffer, accepts those < 9, and
// maps an accepted nibble n to the coefficient 4 - n, producing values in
// [-4, 4]. Fills the output array r[256] with the accepted coefficients in
// order of appearance, returning the total number written (max 256).
//
// extern uint64_t mldsa_rej_uniform_eta4_VARIABLE_TIME_x86
//                     (int32_t r[static 256],
//                      const uint8_t buf[static 272],
//                      const uint8_t table[static 2048]);
//
// The input buffer buf is a fixed MLD_AVX2_REJ_UNIFORM_ETA4_BUFLEN = 272
// bytes; the table argument is a specific precomputed 256x8 = 2048-byte table
// (mldsa_rej_uniform_table), defined in the proof / test code.
//
// Like mlkem_rej_uniform, this is *not* a constant-time function: the time
// taken depends on how many buffer elements pass the < 9 filter.
//
// Standard x86-64 ABI: RDI = r, RSI = buf, RDX = table
// Microsoft x64 ABI:   RCX = r, RDX = buf, R8 = table
// ----------------------------------------------------------------------------

// This implementation is derived from the public domain AVX2 Dilithium
// implementation (pq-crystals/dilithium) via the mldsa-native project.

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_rej_uniform_eta4_VARIABLE_TIME_x86)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_rej_uniform_eta4_VARIABLE_TIME_x86)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_rej_uniform_eta4_VARIABLE_TIME_x86)
        .text
        .balign 4

S2N_BN_SYMBOL(mldsa_rej_uniform_eta4_VARIABLE_TIME_x86):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        // Windows ABI: preserve %xmm6 (nonvolatile, used below) and the
        // nonvolatile GPRs %rdi/rsi that hold the mapped SysV arguments.
        CFI_DEC_RSP(40)
        CFI_STACKSAVEU(%xmm6,0)
        CFI_STACKSAVE(%rdi,16)
        CFI_STACKSAVE(%rsi,24)
        movq    %rcx, %rdi
        movq    %rdx, %rsi
        movq    %r8, %rdx
#endif

        movl    $0xf0f0f0f, %r8d
        vmovd   %r8d, %xmm2
        vpbroadcastd %xmm2, %ymm2
        movl    $0x4040404, %r8d
        vmovd   %r8d, %xmm3
        vpbroadcastd %xmm3, %ymm3
        movl    $0x9090909, %r8d
        vmovd   %r8d, %xmm4
        vpbroadcastd %xmm4, %ymm4
        xorl    %eax, %eax
        xorl    %ecx, %ecx

Lmldsa_rej_uniform_eta4_x86_loop:
        cmpl    $0xf8, %eax
        ja      Lmldsa_rej_uniform_eta4_x86_scalar
        cmpl    $0x100, %ecx
        ja      Lmldsa_rej_uniform_eta4_x86_scalar
        vpmovzxbw (%rsi,%rcx), %ymm0
        vpsllw  $0x4, %ymm0, %ymm1
        vpor    %ymm1, %ymm0, %ymm0
        vpand   %ymm2, %ymm0, %ymm0
        vpsubb  %ymm4, %ymm0, %ymm1
        vpsubb  %ymm0, %ymm3, %ymm0
        vpmovmskb %ymm1, %r8d
        vextracti128 $0x0, %ymm0, %xmm5
        movzx   %r8b, %r10d
        vmovq   (%rdx,%r10,8), %xmm6
        vpshufb %xmm6, %xmm5, %xmm6
        vpmovsxbd %xmm6, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcnt  %r10d, %r9d
        addl    %r9d, %eax
        shrl    $0x8, %r8d
        addl    $0x4, %ecx
        cmpl    $0xf8, %eax
        ja      Lmldsa_rej_uniform_eta4_x86_scalar
        vpsrldq $0x8, %xmm5, %xmm5
        movzx   %r8b, %r10d
        vmovq   (%rdx,%r10,8), %xmm6
        vpshufb %xmm6, %xmm5, %xmm6
        vpmovsxbd %xmm6, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcnt  %r10d, %r9d
        addl    %r9d, %eax
        shrl    $0x8, %r8d
        addl    $0x4, %ecx
        cmpl    $0xf8, %eax
        ja      Lmldsa_rej_uniform_eta4_x86_scalar
        vextracti128 $0x1, %ymm0, %xmm5
        movzx   %r8b, %r10d
        vmovq   (%rdx,%r10,8), %xmm6
        vpshufb %xmm6, %xmm5, %xmm6
        vpmovsxbd %xmm6, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcnt  %r10d, %r9d
        addl    %r9d, %eax
        shrl    $0x8, %r8d
        addl    $0x4, %ecx
        cmpl    $0xf8, %eax
        ja      Lmldsa_rej_uniform_eta4_x86_scalar
        vpsrldq $0x8, %xmm5, %xmm5
        movzx   %r8b, %r10d
        vmovq   (%rdx,%r10,8), %xmm6
        vpshufb %xmm6, %xmm5, %xmm6
        vpmovsxbd %xmm6, %ymm1
        vmovdqu %ymm1, (%rdi,%rax,4)
        popcnt  %r10d, %r9d
        addl    %r9d, %eax
        addl    $0x4, %ecx
        jmp     Lmldsa_rej_uniform_eta4_x86_loop

Lmldsa_rej_uniform_eta4_x86_scalar:
        cmpl    $0x100, %eax
        jae     Lmldsa_rej_uniform_eta4_x86_done
        cmpl    $0x110, %ecx
        jae     Lmldsa_rej_uniform_eta4_x86_done
        movzbl (%rsi,%rcx), %r11d
        incl    %ecx
        movl    %r11d, %r10d
        andl    $0xf, %r10d
        cmpl    $0x9, %r10d
        jae     Lmldsa_rej_uniform_eta4_x86_high_nibble
        movl    $0x4, %r9d
        subl    %r10d, %r9d
        movl    %r9d, (%rdi,%rax,4)
        incl    %eax
        cmpl    $0x100, %eax
        jae     Lmldsa_rej_uniform_eta4_x86_done

Lmldsa_rej_uniform_eta4_x86_high_nibble:
        shrl    $0x4, %r11d
        andl    $0xf, %r11d
        cmpl    $0x9, %r11d
        jae     Lmldsa_rej_uniform_eta4_x86_scalar
        movl    $0x4, %r10d
        subl    %r11d, %r10d
        movl    %r10d, (%rdi,%rax,4)
        incl    %eax
        jmp     Lmldsa_rej_uniform_eta4_x86_scalar

Lmldsa_rej_uniform_eta4_x86_done:

#if WINDOWS_ABI
        CFI_STACKLOADU(%xmm6,0)
        CFI_STACKLOAD(%rdi,16)
        CFI_STACKLOAD(%rsi,24)
        CFI_INC_RSP(40)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_rej_uniform_eta4_VARIABLE_TIME_x86)

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
