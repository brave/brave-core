// SPDX-License-Identifier: Apache-2.0 OR MIT

/*
Refs:
- RISC-V Instruction Set Manual
  Machine Status (mstatus and mstatush) Registers
  https://github.com/riscv/riscv-isa-manual/blob/riscv-isa-release-56e76be-2025-08-26/src/machine.adoc#machine-status-mstatus-and-mstatush-registers
  Supervisor Status (sstatus) Register
  https://github.com/riscv/riscv-isa-manual/blob/riscv-isa-release-56e76be-2025-08-26/src/supervisor.adoc#supervisor-status-sstatus-register

See also src/imp/riscv.rs.

Generated asm:
- riscv64gc https://godbolt.org/z/zTrzT1Ee7
*/

#[cfg(not(portable_atomic_no_asm))]
use core::arch::asm;

#[cfg_attr(
    portable_atomic_no_cfg_target_has_atomic,
    cfg(any(test, portable_atomic_no_atomic_cas))
)]
#[cfg_attr(
    not(portable_atomic_no_cfg_target_has_atomic),
    cfg(any(test, not(target_has_atomic = "ptr")))
)]
pub(super) use super::super::riscv as atomic;

// Status register
#[cfg(not(portable_atomic_s_mode))]
macro_rules! status {
    () => {
        "mstatus"
    };
}
#[cfg(portable_atomic_s_mode)]
macro_rules! status {
    () => {
        "sstatus"
    };
}

// MIE (Machine Interrupt Enable) bit (1 << 3)
#[cfg(not(portable_atomic_s_mode))]
macro_rules! mask {
    () => {
        "0x8"
    };
}
// SIE (Supervisor Interrupt Enable) bit (1 << 1)
#[cfg(portable_atomic_s_mode)]
macro_rules! mask {
    () => {
        "0x2"
    };
}

#[cfg(target_arch = "riscv32")]
pub(crate) type State = u32;
#[cfg(target_arch = "riscv64")]
pub(crate) type State = u64;

/// Disables interrupts and returns the previous interrupt state.
#[inline(always)]
pub(crate) fn disable() -> State {
    let status: State;
    // SAFETY: reading mstatus/sstatus and disabling interrupts is safe.
    // (see module-level comments of interrupt/mod.rs on the safety of using privileged instructions)
    unsafe {
        asm!(
            concat!("csrrci {status}, ", status!(), ", ", mask!()), // atomic { status = status!(); status!() &= !mask!() }
            status = out(reg) status,
            // Do not use `nomem` and `readonly` because prevent subsequent memory accesses from being reordered before interrupts are disabled.
            options(nostack, preserves_flags),
        );
    }
    status
}

/// Restores the previous interrupt state.
///
/// # Safety
///
/// The state must be the one retrieved by the previous `disable`.
#[inline(always)]
pub(crate) unsafe fn restore(prev_status: State) {
    // SAFETY: the caller must guarantee that the state was retrieved by the previous `disable`,
    //
    // This clobbers the entire mstatus/sstatus register. See msp430.rs to safety on this.
    unsafe {
        asm!(
            concat!("csrw ", status!(), ", {prev_status}"), // atomic { status!() = prev_status }
            prev_status = in(reg) prev_status,
            // Do not use `nomem` and `readonly` because prevent preceding memory accesses from being reordered after interrupts are enabled.
            options(nostack, preserves_flags),
        );
    }
}
