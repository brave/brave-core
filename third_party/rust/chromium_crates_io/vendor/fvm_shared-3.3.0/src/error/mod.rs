// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use std::fmt::Formatter;

use num_derive::FromPrimitive;
use serde::{Deserialize, Serialize};
use thiserror::Error;

/// ExitCode defines the exit code from the VM invocation.
#[derive(PartialEq, Eq, Debug, Clone, Copy, Serialize, Deserialize)]
#[serde(transparent)]
#[repr(transparent)]
pub struct ExitCode {
    value: u32,
}

impl ExitCode {
    pub const fn new(value: u32) -> Self {
        Self { value }
    }

    pub fn value(self) -> u32 {
        self.value
    }

    /// Returns true if the exit code indicates success.
    pub fn is_success(self) -> bool {
        self.value == 0
    }

    /// Returns true if the error code is in the range of exit codes reserved for the VM
    /// (including Ok).
    pub fn is_system_error(self) -> bool {
        self.value < (Self::FIRST_USER_EXIT_CODE)
    }
}

impl From<u32> for ExitCode {
    fn from(value: u32) -> Self {
        ExitCode { value }
    }
}

impl std::fmt::Display for ExitCode {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.value)
    }
}

impl ExitCode {
    // Exit codes which originate inside the VM.
    // These values may not be used by actors when aborting.

    /// The code indicating successful execution.
    pub const OK: ExitCode = ExitCode::new(0);
    /// The message sender doesn't exist.
    pub const SYS_SENDER_INVALID: ExitCode = ExitCode::new(1);
    /// The message sender was not in a valid state to send this message.
    ///
    /// Either:
    /// - The sender's nonce nonce didn't match the message nonce.
    /// - The sender didn't have the funds to cover the message gas.
    pub const SYS_SENDER_STATE_INVALID: ExitCode = ExitCode::new(2);
    //pub const SYS_RESERVED_3 ExitCode = ExitCode::new(3);
    /// The message receiver trapped (panicked).
    pub const SYS_ILLEGAL_INSTRUCTION: ExitCode = ExitCode::new(4);
    /// The message receiver doesn't exist and can't be automatically created
    pub const SYS_INVALID_RECEIVER: ExitCode = ExitCode::new(5);
    /// The message sender didn't have the requisite funds.
    pub const SYS_INSUFFICIENT_FUNDS: ExitCode = ExitCode::new(6);
    /// Message execution (including subcalls) used more gas than the specified limit.
    pub const SYS_OUT_OF_GAS: ExitCode = ExitCode::new(7);
    // pub const SYS_RESERVED_8: ExitCode = ExitCode::new(8);
    /// The message receiver aborted with a reserved exit code.
    pub const SYS_ILLEGAL_EXIT_CODE: ExitCode = ExitCode::new(9);
    /// An internal VM assertion failed.
    pub const SYS_ASSERTION_FAILED: ExitCode = ExitCode::new(10);
    /// The actor returned a block handle that doesn't exist
    pub const SYS_MISSING_RETURN: ExitCode = ExitCode::new(11);
    // pub const SYS_RESERVED_12: ExitCode = ExitCode::new(12);
    // pub const SYS_RESERVED_13: ExitCode = ExitCode::new(13);
    // pub const SYS_RESERVED_14: ExitCode = ExitCode::new(14);
    // pub const SYS_RESERVED_15: ExitCode = ExitCode::new(15);

    /// The lowest exit code that an actor may abort with.
    pub const FIRST_USER_EXIT_CODE: u32 = 16;

    // Standard exit codes according to the built-in actors' calling convention.
    /// The method parameters are invalid.
    pub const USR_ILLEGAL_ARGUMENT: ExitCode = ExitCode::new(16);
    /// The requested resource does not exist.
    pub const USR_NOT_FOUND: ExitCode = ExitCode::new(17);
    /// The requested operation is forbidden.
    pub const USR_FORBIDDEN: ExitCode = ExitCode::new(18);
    /// The actor has insufficient funds to perform the requested operation.
    pub const USR_INSUFFICIENT_FUNDS: ExitCode = ExitCode::new(19);
    /// The actor's internal state is invalid.
    pub const USR_ILLEGAL_STATE: ExitCode = ExitCode::new(20);
    /// There was a de/serialization failure within actor code.
    pub const USR_SERIALIZATION: ExitCode = ExitCode::new(21);
    /// The message cannot be handled (usually indicates an unhandled method number).
    pub const USR_UNHANDLED_MESSAGE: ExitCode = ExitCode::new(22);
    /// The actor failed with an unspecified error.
    pub const USR_UNSPECIFIED: ExitCode = ExitCode::new(23);
    /// The actor failed a user-level assertion.
    pub const USR_ASSERTION_FAILED: ExitCode = ExitCode::new(24);
    /// The requested operation cannot be performed in "read-only" mode.
    pub const USR_READ_ONLY: ExitCode = ExitCode::new(25);
    /// The method cannot handle a transfer of value.
    pub const USR_NOT_PAYABLE: ExitCode = ExitCode::new(26);
    // pub const RESERVED_27: ExitCode = ExitCode::new(27);
    // pub const RESERVED_28: ExitCode = ExitCode::new(28);
    // pub const RESERVED_29: ExitCode = ExitCode::new(29);
    // pub const RESERVED_30: ExitCode = ExitCode::new(30);
    // pub const RESERVED_31: ExitCode = ExitCode::new(31);
}

/// When a syscall fails, it returns an `ErrorNumber` to indicate why. The syscalls themselves
/// include documentation on _which_ syscall errors they can be expected to return, and what they
/// mean in the context of the syscall.
#[non_exhaustive]
#[repr(u32)]
#[derive(Copy, Clone, Eq, Debug, PartialEq, Error, FromPrimitive)]
pub enum ErrorNumber {
    /// A syscall parameters was invalid.
    IllegalArgument = 1,
    /// The actor is not in the correct state to perform the requested operation.
    IllegalOperation = 2,
    /// This syscall would exceed some system limit (memory, lookback, call depth, etc.).
    LimitExceeded = 3,
    /// A system-level assertion has failed.
    ///
    /// # Note
    ///
    /// Non-system actors should never receive this error number. A system-level assertion will
    /// cause the entire message to fail.
    AssertionFailed = 4,
    /// There were insufficient funds to complete the requested operation.
    InsufficientFunds = 5,
    /// A resource was not found.
    NotFound = 6,
    /// The specified IPLD block handle was invalid.
    InvalidHandle = 7,
    /// The requested CID shape (multihash codec, multihash length) isn't supported.
    IllegalCid = 8,
    /// The requested IPLD codec isn't supported.
    IllegalCodec = 9,
    /// The IPLD block did not match the specified IPLD codec.
    Serialization = 10,
    /// The operation is forbidden.
    Forbidden = 11,
    /// The passed buffer is too small.
    BufferTooSmall = 12,
    /// The actor is executing in a read-only context.
    ReadOnly = 13,
}

impl std::fmt::Display for ErrorNumber {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        use ErrorNumber::*;
        f.write_str(match *self {
            IllegalArgument => "illegal argument",
            IllegalOperation => "illegal operation",
            LimitExceeded => "limit exceeded",
            AssertionFailed => "filecoin assertion failed",
            InsufficientFunds => "insufficient funds",
            NotFound => "resource not found",
            InvalidHandle => "invalid ipld block handle",
            IllegalCid => "illegal cid specification",
            IllegalCodec => "illegal ipld codec",
            Serialization => "serialization error",
            Forbidden => "operation forbidden",
            BufferTooSmall => "buffer too small",
            ReadOnly => "execution context is read-only",
        })
    }
}
