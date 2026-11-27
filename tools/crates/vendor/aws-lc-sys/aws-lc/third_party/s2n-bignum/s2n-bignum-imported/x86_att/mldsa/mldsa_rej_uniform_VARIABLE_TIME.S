// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

// ----------------------------------------------------------------------------
// Uniform rejection sampling for ML-DSA
// Input buf[840] (uint8_t); output r[256] (int32_t); table[2048] (uint64_t)
// Returns: number of valid coefficients in r (at most 256)
//
// This function implements the rejection-sampling loop for ML-DSA, extracting
// 23-bit coefficients from packed 24-bit input bytes and keeping only those
// strictly less than q = 8380417. A main AVX2 loop processes 24 bytes (8
// coefficients) per iteration using VPERMQ+VPSHUFB extraction, VPAND masking,
// VPSUBD+VMOVMSKPS rejection, and VPERMD+table compaction. A scalar tail
// handles any remaining bytes after the main loop exits.
//
// This implementation is derived from the public domain AVX2 Dilithium
// implementation from CRYSTALS-Dilithium optimized AVX2 implementation by
// Bai, Ducas, Kiltz, Lepoint, Lyubashevsky, Schwabe, Seiler, Stehlé
// (https://github.com/pq-crystals/dilithium/tree/master/avx2)
//
// extern uint32_t mldsa_rej_uniform_VARIABLE_TIME_x86
//   (int32_t r[static 256],
//    const uint8_t buf[static 840],
//    const uint64_t table[static 256]);
//
// Standard x86-64 ABI: RDI = r, RSI = buf, RDX = table
// Microsoft x64 ABI:   RCX = r, RDX = buf, R8 = table
// ----------------------------------------------------------------------------

#include "_internal_s2n_bignum_x86_att.h"


        S2N_BN_SYM_VISIBILITY_DIRECTIVE(mldsa_rej_uniform_VARIABLE_TIME_x86)
        S2N_BN_FUNCTION_TYPE_DIRECTIVE(mldsa_rej_uniform_VARIABLE_TIME_x86)
        S2N_BN_SYM_PRIVACY_DIRECTIVE(mldsa_rej_uniform_VARIABLE_TIME_x86)
        .text
        .balign 32

S2N_BN_SYMBOL(mldsa_rej_uniform_VARIABLE_TIME_x86):
        CFI_START
        _CET_ENDBR

#if WINDOWS_ABI
        // The body only uses the volatile registers %xmm0-xmm4 (%ymm0-ymm4) and
        // writes results directly to the output buffer, so none of the
        // nonvolatile %xmm6-xmm15 need saving. We only preserve %rdi/rsi, which
        // hold the mapped SysV argument pointers.
        CFI_PUSH(%rdi)
        CFI_PUSH(%rsi)
        movq    %rcx, %rdi
        movq    %rdx, %rsi
        movq    %r8, %rdx
#endif

        // Shuffle mask: expand 24 bytes (8 x 3-byte coefficients) into
        // 8 x 4-byte lanes (with a zero high byte in each).
        movq    $0xff050403ff020100, %r10
        vmovq   %r10, %xmm0
        movq    $0xff0b0a09ff080706, %r10
        vpinsrq $0x1, %r10, %xmm0, %xmm0
        movq    $0xff090807ff060504, %r10
        vmovq   %r10, %xmm3
        movq    $0xff0f0e0dff0c0b0a, %r10
        vpinsrq $0x1, %r10, %xmm3, %xmm3
        vinserti128 $0x1, %xmm3, %ymm0, %ymm0

        // Mask 0x7fffff in all 8 lanes (keep low 23 bits)
        movl    $0x7fffff, %r8d
        vmovd   %r8d, %xmm1
        vpbroadcastd %xmm1, %ymm1

        // Threshold q = 0x7fe001 in all 8 lanes
        movl    $0x7fe001, %r8d
        vmovd   %r8d, %xmm2
        vpbroadcastd %xmm2, %ymm2

        // %rax = accepted count, %rcx = byte offset into buf
        xorl    %eax, %eax
        xorl    %ecx, %ecx

Lmldsa_rej_uniform_VARIABLE_TIME_x86_loop:
        // Exit to scalar tail if we have 248 or more accepted (next 8 might
        // overflow) or if byte offset is past 808 (would read past buf+840-24).
        cmpl    $0xf8, %eax
        ja      Lmldsa_rej_uniform_VARIABLE_TIME_x86_scalar
        cmpl    $0x328, %ecx
        ja      Lmldsa_rej_uniform_VARIABLE_TIME_x86_scalar

        vmovdqu  (%rsi,%rcx), %ymm3
        addl    $0x18, %ecx
        vpermq  $0x94, %ymm3, %ymm3
        vpshufb %ymm0, %ymm3, %ymm3
        vpand   %ymm1, %ymm3, %ymm3
        vpsubd  %ymm2, %ymm3, %ymm4
        vmovmskps %ymm4, %r8d
        popcnt  %r8d, %r9d
        vmovq    (%rdx,%r8,8), %xmm4
        vpmovzxbd %xmm4, %ymm4
        vpermd  %ymm3, %ymm4, %ymm3
        vmovdqu %ymm3,  (%rdi,%rax,4)
        addl    %r9d, %eax
        jmp     Lmldsa_rej_uniform_VARIABLE_TIME_x86_loop

Lmldsa_rej_uniform_VARIABLE_TIME_x86_scalar:
        cmpl    $0x100, %eax
        jae     Lmldsa_rej_uniform_VARIABLE_TIME_x86_done
        cmpl    $0x345, %ecx
        ja      Lmldsa_rej_uniform_VARIABLE_TIME_x86_done
        movzwl (%rsi,%rcx), %r8d
        movzbl 2(%rsi,%rcx,1), %r9d
        shll    $0x10, %r9d
        orl     %r9d, %r8d
        andl    $0x7fffff, %r8d
        addl    $0x3, %ecx
        cmpl    $0x7fe001, %r8d
        jae     Lmldsa_rej_uniform_VARIABLE_TIME_x86_scalar
        movl    %r8d,  (%rdi,%rax,4)
        addl    $0x1, %eax
        jmp     Lmldsa_rej_uniform_VARIABLE_TIME_x86_scalar

Lmldsa_rej_uniform_VARIABLE_TIME_x86_done:
#if WINDOWS_ABI
        CFI_POP(%rsi)
        CFI_POP(%rdi)
#endif
        CFI_RET

S2N_BN_SIZE_DIRECTIVE(mldsa_rej_uniform_VARIABLE_TIME_x86)

#if defined(__linux__) && defined(__ELF__)
        .section .note.GNU-stack,"",%progbits
#endif
