// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
//! This module contains types exchanged at the syscall layer between actors
//! (usually through the SDK) and the FVM.

use bitflags::bitflags;
use num_bigint::TryFromBigIntError;

pub mod out;

pub type BlockId = u32;
pub type Codec = u64;

/// The token amount type used in syscalls. It can represent any token amount (in atto-FIL) from 0
/// to `2^128-1` attoFIL. Or 0 to about 340 exaFIL.
///
/// Internally, this type is a tuple of `u64`s storing the "low" and "high" bits of a little-endian
/// u128.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(packed, C)]
pub struct TokenAmount {
    pub lo: u64,
    pub hi: u64,
}

impl From<TokenAmount> for crate::econ::TokenAmount {
    fn from(v: TokenAmount) -> Self {
        crate::econ::TokenAmount::from_atto((v.hi as u128) << 64 | (v.lo as u128))
    }
}

impl TryFrom<crate::econ::TokenAmount> for TokenAmount {
    type Error = TryFromBigIntError<()>;
    fn try_from(v: crate::econ::TokenAmount) -> Result<Self, Self::Error> {
        v.atto().try_into().map(|v: u128| Self {
            hi: (v >> u64::BITS) as u64,
            lo: v as u64,
        })
    }
}

impl<'a> TryFrom<&'a crate::econ::TokenAmount> for TokenAmount {
    type Error = TryFromBigIntError<()>;
    fn try_from(v: &'a crate::econ::TokenAmount) -> Result<Self, Self::Error> {
        v.atto().try_into().map(|v: u128| Self {
            hi: (v >> u64::BITS) as u64,
            lo: v as u64,
        })
    }
}

bitflags! {
    /// Flags passed to the send syscall.
    #[derive(Default)]
    #[repr(transparent)]
    // note: this is 64 bits because I don't want to hate my past self, not because we need them
    // right now. It doesn't really cost anything anyways.
    pub struct SendFlags: u64 {
        /// Send in "read-only" mode.
        const READ_ONLY = 0b00000001;
    }
}

impl SendFlags {
    pub fn read_only(self) -> bool {
        self.intersects(Self::READ_ONLY)
    }
}

/// An unsafe trait to mark "syscall safe" types. These types must be safe to memcpy to and from
/// WASM. This means:
///
/// 1. Repr C & packed alignment (no reordering, no padding).
/// 2. Copy, Sized, and no pointers.
/// 3. No floats (non-determinism).
///
/// # Safety
///
/// Incorrectly implementing this could lead to undefined behavior in types passed between wasm and
/// rust.
pub unsafe trait SyscallSafe: Copy + Sized + 'static {}

macro_rules! assert_syscall_safe {
    ($($t:ty,)*) => {
        $(unsafe impl SyscallSafe for $t {})*
    }
}

assert_syscall_safe! {
    (),

    u8, u16, u32, u64,
    i8, i16, i32, i64,

    TokenAmount,
    out::ipld::IpldOpen,
    out::ipld::IpldStat,
    out::send::Send,
    out::crypto::VerifyConsensusFault,
    out::network::NetworkContext,
    out::vm::MessageContext,
}

unsafe impl<T, const N: usize> SyscallSafe for [T; N] where T: SyscallSafe {}
