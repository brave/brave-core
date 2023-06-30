//! This module contains types exchanged at the syscall layer between actors
//! (usually through the SDK) and the FVM.

pub mod out;

pub type BlockId = u32;
pub type Codec = u64;

/// The token amount type used in syscalls. It can represent any token amount (in atto-FIL) from 0
/// to `2^128-1` attoFIL. Or 0 to about 340 exaFIL.
///
/// Internally, this type is a tuple of `u64`s storing the "low" and "high" bits of a little-endian
/// u128.
#[derive(Debug, Copy, Clone)]
#[repr(packed, C)]
pub struct TokenAmount {
    pub lo: u64,
    pub hi: u64,
}

impl From<TokenAmount> for crate::econ::TokenAmount {
    fn from(v: TokenAmount) -> Self {
        crate::econ::TokenAmount::from(v.hi) << 64 | crate::econ::TokenAmount::from(v.lo)
    }
}

impl TryFrom<crate::econ::TokenAmount> for TokenAmount {
    type Error = <crate::econ::TokenAmount as TryInto<u128>>::Error;
    fn try_from(v: crate::econ::TokenAmount) -> Result<Self, Self::Error> {
        v.try_into().map(|v: u128| Self {
            hi: (v >> u64::BITS) as u64,
            lo: v as u64,
        })
    }
}

impl<'a> TryFrom<&'a crate::econ::TokenAmount> for TokenAmount {
    type Error = <&'a crate::econ::TokenAmount as TryInto<u128>>::Error;
    fn try_from(v: &'a crate::econ::TokenAmount) -> Result<Self, Self::Error> {
        v.try_into().map(|v: u128| Self {
            hi: (v >> u64::BITS) as u64,
            lo: v as u64,
        })
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
    out::vm::InvocationContext,
}

unsafe impl<T, const N: usize> SyscallSafe for [T; N] where T: SyscallSafe {}
