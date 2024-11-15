// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
//! This module contains syscall output data carrier structs, shared between
//! the FVM SDK and the FVM itself, wrapping multi-value returns.
//!
//! These are necessary because Rust WASM multi-value return compilation is
//! plagued with issues and catch-22 problems, making it unfeasible to use
//! actual bare multi-value returns in FFI extern definitions.
//!
//! Read more at https://github.com/rust-lang/rust/issues/73755.

// NOTE: When possible, pack fields such that loads will be power-of-two aligned. Un-aligned loads
// _can_ be done (LLVM will generate the appropriate code) but are slower.
//
// Read up on https://doc.rust-lang.org/reference/type-layout.html for more information.
//
// Also, please also read the docs on super::SyscallSafe before modifying any of these types.

pub mod ipld {
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    #[repr(packed, C)]
    pub struct IpldOpen {
        pub codec: u64,
        pub id: u32,
        pub size: u32,
    }

    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    #[repr(packed, C)]
    pub struct IpldStat {
        pub codec: u64,
        pub size: u32,
    }
}

pub mod send {
    use crate::sys::BlockId;

    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    #[repr(packed, C)]
    pub struct Send {
        pub exit_code: u32,
        pub return_id: BlockId,
        pub return_codec: u64,
        pub return_size: u32,
    }
}

pub mod crypto {
    use crate::{ActorID, ChainEpoch};

    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    #[repr(packed, C)]
    pub struct VerifyConsensusFault {
        pub epoch: ChainEpoch,
        pub target: ActorID,
        pub fault: u32,
    }
}

pub mod vm {
    use bitflags::bitflags;

    use crate::sys::TokenAmount;
    use crate::{ActorID, MethodNum};

    bitflags! {
        /// Invocation flags pertaining to the currently executing actor.
        #[derive(Default, Copy, Clone, Eq, PartialEq, Debug)]
        #[repr(transparent)]
        pub struct ContextFlags: u64 {
            /// Invocation is in "read-only" mode. Any balance transfers, sends that would create
            /// actors, and calls to `sself::set_root` and `sself::self_destruct` will be rejected.
            const READ_ONLY = 0b00000001;
        }
    }

    impl ContextFlags {
        pub fn read_only(self) -> bool {
            self.intersects(Self::READ_ONLY)
        }
    }

    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    #[repr(packed, C)]
    pub struct MessageContext {
        /// The current call's origin actor ID.
        pub origin: ActorID,
        /// The nonce from the explicit message.
        pub nonce: u64,
        /// The caller's actor ID.
        pub caller: ActorID,
        /// The receiver's actor ID (i.e. ourselves).
        pub receiver: ActorID,
        /// The method number from the message.
        pub method_number: MethodNum,
        /// The value that was received.
        pub value_received: TokenAmount,
        /// The gas premium being paid by the currently executing message (on top of the base-fee).
        /// This may be less than the premium specified in the message if the base fee plus the
        /// premium would exceed the fee cap.
        pub gas_premium: TokenAmount,
        /// Flags pertaining to the currently executing actor's invocation context.
        pub flags: ContextFlags,
    }
}

pub mod network {
    use crate::clock::ChainEpoch;
    use crate::sys::TokenAmount;
    use crate::version::NetworkVersion;

    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    #[repr(packed, C)]
    pub struct NetworkContext {
        /// The current epoch.
        pub epoch: ChainEpoch,
        /// The current time (seconds since the unix epoch).
        pub timestamp: u64,
        /// The current base-fee.
        pub base_fee: TokenAmount,
        /// The Chain ID of the network.
        pub chain_id: u64,
        /// The network version.
        pub network_version: NetworkVersion,
    }
}
