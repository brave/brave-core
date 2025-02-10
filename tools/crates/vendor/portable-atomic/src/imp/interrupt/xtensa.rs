// SPDX-License-Identifier: Apache-2.0 OR MIT

/*
Refs:
- Xtensa Instruction Set Architecture (ISA) Summary for all Xtensa LX Processors
  https://www.cadence.com/content/dam/cadence-www/global/en_US/documents/tools/silicon-solutions/compute-ip/isa-summary.pdf
- Linux kernel's Xtensa atomic implementation
  https://github.com/torvalds/linux/blob/v6.11/arch/xtensa/include/asm/atomic.h
*/

use core::arch::asm;

pub(super) use core::sync::atomic;

pub(super) type State = u32;

/// Disables interrupts and returns the previous interrupt state.
#[inline(always)]
pub(super) fn disable() -> State {
    let ps: State;
    // SAFETY: reading the PS special register and disabling all interrupts is safe.
    // (see module-level comments of interrupt/mod.rs on the safety of using privileged instructions)
    unsafe {
        // Do not use `nomem` and `readonly` because prevent subsequent memory accesses from being reordered before interrupts are disabled.
        // Interrupt level 15 to disable all interrupts.
        // SYNC after RSIL is not required.
        asm!("rsil {0}, 15", out(reg) ps, options(nostack));
    }
    ps
}

/// Restores the previous interrupt state.
///
/// # Safety
///
/// The state must be the one retrieved by the previous `disable`.
#[inline(always)]
pub(super) unsafe fn restore(ps: State) {
    // SAFETY: the caller must guarantee that the state was retrieved by the previous `disable`,
    // and we've checked that interrupts were enabled before disabling interrupts.
    unsafe {
        // Do not use `nomem` and `readonly` because prevent preceding memory accesses from being reordered after interrupts are enabled.
        // SYNC after WSR is required to guarantee that subsequent RSIL read the written value.
        asm!(
            "wsr.ps {0}",
            "rsync",
            in(reg) ps,
            options(nostack),
        );
    }
}
