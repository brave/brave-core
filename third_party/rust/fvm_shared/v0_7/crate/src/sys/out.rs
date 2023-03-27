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
    #[derive(Debug, Copy, Clone)]
    #[repr(packed, C)]
    pub struct IpldOpen {
        pub codec: u64,
        pub id: u32,
        pub size: u32,
    }

    #[derive(Debug, Copy, Clone)]
    #[repr(packed, C)]
    pub struct IpldStat {
        pub codec: u64,
        pub size: u32,
    }
}

pub mod send {
    use crate::sys::BlockId;

    #[derive(Debug, Copy, Clone)]
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

    #[derive(Debug, Copy, Clone)]
    #[repr(packed, C)]
    pub struct VerifyConsensusFault {
        pub epoch: ChainEpoch,
        pub target: ActorID,
        pub fault: u32,
    }
}

pub mod vm {
    use crate::clock::ChainEpoch;
    use crate::sys::TokenAmount;
    use crate::{ActorID, MethodNum};

    #[derive(Debug, Copy, Clone)]
    #[repr(packed, C)]
    pub struct InvocationContext {
        /// The value that was received.
        pub value_received: TokenAmount,
        /// The caller's actor ID.
        pub caller: ActorID,
        /// The receiver's actor ID (i.e. ourselves).
        pub receiver: ActorID,
        /// The method number from the message.
        pub method_number: MethodNum,
        /// The current epoch.
        pub network_curr_epoch: ChainEpoch,
        /// The network version.
        pub network_version: u32,
    }
}
