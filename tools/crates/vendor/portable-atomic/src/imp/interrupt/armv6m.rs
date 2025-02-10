// SPDX-License-Identifier: Apache-2.0 OR MIT

/*
Adapted from https://github.com/rust-embedded/cortex-m.

Generated asm:
- armv6-m https://godbolt.org/z/1sqKnsY6n
*/

#[cfg(not(portable_atomic_no_asm))]
use core::arch::asm;

pub(super) use core::sync::atomic;

pub(super) type State = u32;

/// Disables interrupts and returns the previous interrupt state.
#[inline(always)]
pub(super) fn disable() -> State {
    let primask: State;
    // SAFETY: reading the priority mask register and disabling interrupts are safe.
    // (see module-level comments of interrupt/mod.rs on the safety of using privileged instructions)
    unsafe {
        // Do not use `nomem` and `readonly` because prevent subsequent memory accesses from being reordered before interrupts are disabled.
        asm!(
            "mrs {0}, PRIMASK",
            "cpsid i",
            out(reg) primask,
            options(nostack, preserves_flags),
        );
    }
    primask
}

/// Restores the previous interrupt state.
///
/// # Safety
///
/// The state must be the one retrieved by the previous `disable`.
#[inline(always)]
pub(super) unsafe fn restore(primask: State) {
    if primask & 0x1 == 0 {
        // SAFETY: the caller must guarantee that the state was retrieved by the previous `disable`,
        // and we've checked that interrupts were enabled before disabling interrupts.
        unsafe {
            // Do not use `nomem` and `readonly` because prevent preceding memory accesses from being reordered after interrupts are enabled.
            asm!("cpsie i", options(nostack, preserves_flags));
        }
    }
}
