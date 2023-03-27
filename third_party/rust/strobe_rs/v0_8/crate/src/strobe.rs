use crate::keccak::{keccakf_u8, AlignedKeccakState, KECCAK_BLOCK_BITLEN_STR, KECCAK_BLOCK_SIZE};

use bitflags::bitflags;
use subtle::{self, ConstantTimeEq};
use zeroize::{Zeroize, ZeroizeOnDrop};

// With this feature on, a user can serialize and deserialize the state of a STROBE session
#[cfg(feature = "serialize_secret_state")]
use serde::{Deserialize, Serialize};

/// Version of Strobe that this crate implements.
pub const STROBE_VERSION: &[u8] = b"1.0.2";

/// A placeholder for STROBE version strings. This is the length of the real version strings, for
/// Keccak-f[1600]
const TEMPLATE_VERSION_STR: [u8; 29] = *b"Strobe-Keccak-sss/bbbb-vX.Y.Z";

bitflags! {
    /// Operation flags defined in the Strobe paper. This is defined as a bitflags struct.
    #[cfg_attr(feature = "serialize_secret_state", derive(Serialize, Deserialize))]
    pub(crate) struct OpFlags: u8 {
        /// Is data being moved inbound
        const I = 1<<0;
        /// Is data being sent to the application
        const A = 1<<1;
        /// Does this operation use cipher output
        const C = 1<<2;
        /// Is data being sent for transport
        const T = 1<<3;
        /// Use exclusively for metadata operations
        const M = 1<<4;
        /// Reserved and currently unimplemented. Using this will cause a panic.
        const K = 1<<5;
    }
}

impl Zeroize for OpFlags {
    fn zeroize(&mut self) {
        self.bits.zeroize()
    }
}

/// Security parameter. Choice of 128 or 256 bits.
#[derive(Clone, Copy)]
#[cfg_attr(feature = "serialize_secret_state", derive(Serialize, Deserialize))]
#[repr(usize)]
pub enum SecParam {
    B128 = 128,
    B256 = 256,
}

/// An empty struct that just indicates that MAC verification failed
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct AuthError;

impl core::fmt::Display for AuthError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_str("MAC verification failed")
    }
}

/// The main Strobe object. This is currently limited to using Keccak-f\[1600\] as the internal
/// permutation function. For more information on this object, the [protocol specification][spec]
/// is a great resource.
///
/// [spec]: https://strobe.sourceforge.io/specs/
///
/// Description of method input
/// ---------------------------
/// Most operations exposed by `Strobe` take the same set of inputs. The arguments are
///
/// * `data` - The input data to the operation.
/// * `more` - For streaming purposes. Specifies whether you're trying to add more input / get more
///            output to/from the previous operation. For example:
///
/// ```rust
/// # extern crate strobe_rs;
/// # use strobe_rs::{SecParam, Strobe};
/// # fn main() {
/// # let mut s = Strobe::new(b"example-of-more", SecParam::B128);
/// s.ad(b"hello world", false);
/// # }
/// ```
/// is equivalent to
/// ```rust
/// # extern crate strobe_rs;
/// # use strobe_rs::{SecParam, Strobe};
/// # fn main() {
/// # let mut s = Strobe::new(b"example-of-more", SecParam::B128);
/// s.ad(b"hello ", false);
/// s.ad(b"world", true);
/// # }
/// ```
///
/// **NOTE:** If you try to set the `more` flag for an operation that is not preceded by the same
/// operation (e.g., if you try `ad` followed by `send_enc` with `more=true`), then **the function
/// will panic**, since that is an invalid use of the `more` flag.
///
/// Finally, `ratchet` and `meta_ratchet` take a `usize` argument instead of bytes. These functions
/// are individually commented below.
#[derive(Clone, Zeroize, ZeroizeOnDrop)]
#[cfg_attr(feature = "serialize_secret_state", derive(Serialize, Deserialize))]
pub struct Strobe {
    /// Internal Keccak state
    pub(crate) st: AlignedKeccakState,
    /// Security parameter (128 or 256)
    #[zeroize(skip)]
    sec: SecParam,
    /// This is the `R` parameter in the Strobe spec
    rate: usize,
    /// Index into `st`
    pos: usize,
    /// Index into `st`
    pos_begin: usize,
    /// Represents whether we're a sender or a receiver or uninitialized
    is_receiver: Option<bool>,
    /// The last operation performed. This is to verify that the `more` flag is only used across
    /// identical operations.
    prev_flags: Option<OpFlags>,
}

// This defines an operation and meta-operation that mutates its input
macro_rules! def_op_mut {
    ($name:ident, $meta_name:ident, $flags:expr, $doc_str:expr) => {
        #[doc = $doc_str]
        pub fn $name(&mut self, data: &mut [u8], more: bool) {
            let flags = $flags;
            self.operate(flags, data, more);
        }

        #[doc = $doc_str]
        pub fn $meta_name(&mut self, data: &mut [u8], more: bool) {
            let flags = $flags | OpFlags::M;
            self.operate(flags, data, more);
        }
    };
}

// This defines an operation and meta-operation that does not mutate its input
macro_rules! def_op_no_mut {
    ($name:ident, $meta_name:ident, $flags:expr, $doc_str:expr) => {
        #[doc = $doc_str]
        pub fn $name(&mut self, data: &[u8], more: bool) {
            let flags = $flags;
            self.operate_no_mutate(flags, data, more);
        }

        #[doc = $doc_str]
        pub fn $meta_name(&mut self, data: &[u8], more: bool) {
            let flags = $flags | OpFlags::M;
            self.operate_no_mutate(flags, data, more);
        }
    };
}

impl Strobe {
    /// Makes a new `Strobe` object with a given protocol byte string and security parameter.
    pub fn new(proto: &[u8], sec: SecParam) -> Strobe {
        let rate = KECCAK_BLOCK_SIZE * 8 - (sec as usize) / 4 - 2;
        assert!(rate >= 1);
        assert!(rate < 254);

        // Initialize state: st = F([0x01, R+2, 0x01, 0x00, 0x01, 0x60] + b"STROBEvX.Y.Z")
        let mut st_buf = [0u8; KECCAK_BLOCK_SIZE * 8];
        st_buf[0..6].copy_from_slice(&[0x01, (rate as u8) + 2, 0x01, 0x00, 0x01, 0x60]);
        st_buf[6..13].copy_from_slice(b"STROBEv");
        st_buf[13..18].copy_from_slice(STROBE_VERSION);

        let mut st = AlignedKeccakState(st_buf);
        keccakf_u8(&mut st);

        let mut strobe = Strobe {
            st,
            sec,
            rate,
            pos: 0,
            pos_begin: 0,
            is_receiver: None,
            prev_flags: None,
        };

        // Mix the protocol into the state
        strobe.meta_ad(proto, false);

        strobe
    }

    /// Returns a bytestring of the form `Strobe-Keccak-SEC/B-vVER` where `SEC` is the bits of
    /// security (128 or 256), `B` is the block size (in bits) of the Keccak permutation function,
    /// and `VER` is the protocol version.
    pub fn version_str(&self) -> [u8; TEMPLATE_VERSION_STR.len()] {
        let mut buf = TEMPLATE_VERSION_STR;

        match self.sec {
            SecParam::B128 => buf[14..17].copy_from_slice(b"128"),
            SecParam::B256 => buf[14..17].copy_from_slice(b"256"),
        }
        buf[18..22].copy_from_slice(KECCAK_BLOCK_BITLEN_STR);
        buf[24..29].copy_from_slice(STROBE_VERSION);

        buf
    }

    /// Validates that the `more` flag is being used correctly. Panics when validation fails.
    fn validate_streaming(&mut self, flags: OpFlags, more: bool) {
        // Streaming only makes sense if this operation is the same as last. For example you can do
        //     s.ad("hello", false);
        //     s.ad(" world", true).
        // But you can't do
        //     s.ad("hello", false);
        //     s.key(" world", true).
        if more {
            assert_eq!(
                self.prev_flags,
                Some(flags),
                "`more` can only be used when this operation is the same as the previous operation"
            );
        }

        // Update the last-performed operation (i.e., the one we're about to perform)
        self.prev_flags = Some(flags);
    }

    // Runs the permutation function on the internal state
    fn run_f(&mut self) {
        self.st.0[self.pos] ^= self.pos_begin as u8;
        self.st.0[self.pos + 1] ^= 0x04;
        self.st.0[self.rate + 1] ^= 0x80;

        keccakf_u8(&mut self.st);
        self.pos = 0;
        self.pos_begin = 0;
    }

    /// XORs the given data into the state. This is a special case of the `duplex` code in the
    /// STROBE paper.
    fn absorb(&mut self, data: &[u8]) {
        for b in data {
            self.st.0[self.pos] ^= *b;

            self.pos += 1;
            if self.pos == self.rate {
                self.run_f();
            }
        }
    }

    /// XORs the given data into the state, then sets the data equal the state.  This is a special
    /// case of the `duplex` code in the STROBE paper.
    fn absorb_and_set(&mut self, data: &mut [u8]) {
        for b in data {
            let state_byte = self.st.0.get_mut(self.pos).unwrap();
            *state_byte ^= *b;
            *b = *state_byte;

            self.pos += 1;
            if self.pos == self.rate {
                self.run_f();
            }
        }
    }

    /// Copies the internal state into the given buffer. This is a special case of `absorb_and_set`
    /// where `data` is all zeros.
    fn copy_state(&mut self, data: &mut [u8]) {
        for b in data {
            *b = self.st.0[self.pos];

            self.pos += 1;
            if self.pos == self.rate {
                self.run_f();
            }
        }
    }

    /// Overwrites the state with the given data while XORing the given data with the old state.
    /// This is a special case of the `duplex` code in the STROBE paper.
    fn exchange(&mut self, data: &mut [u8]) {
        for b in data {
            let state_byte = self.st.0.get_mut(self.pos).unwrap();
            *b ^= *state_byte;
            *state_byte ^= *b;

            self.pos += 1;
            if self.pos == self.rate {
                self.run_f();
            }
        }
    }

    /// Overwrites the state with the given data. This is a special case of `Strobe::exchange`,
    /// where we do not want to mutate the input data.
    fn overwrite(&mut self, data: &[u8]) {
        for b in data {
            self.st.0[self.pos] = *b;

            self.pos += 1;
            if self.pos == self.rate {
                self.run_f();
            }
        }
    }

    /// Copies the state into the given buffer and sets the state to 0. This is a special case of
    /// `Strobe::exchange`, where `data` is assumed to be the all-zeros string. This is precisely
    /// the case when the current operation is PRF.
    fn squeeze(&mut self, data: &mut [u8]) {
        for b in data {
            let state_byte = self.st.0.get_mut(self.pos).unwrap();
            *b = *state_byte;
            *state_byte = 0;

            self.pos += 1;
            if self.pos == self.rate {
                self.run_f();
            }
        }
    }

    /// Overwrites the state with a specified number of zeros. This is a special case of
    /// `Strobe::exchange`. More specifically, it's a special case of `Strobe::overwrite` and
    /// `Strobe::squeeze`. It's like `squeeze` in that we assume we've been given all zeros as
    /// input, and like `overwrite` in that we do not mutate (or take) any input.
    fn zero_state(&mut self, mut bytes_to_zero: usize) {
        static ZEROS: [u8; 8 * KECCAK_BLOCK_SIZE] = [0u8; 8 * KECCAK_BLOCK_SIZE];

        // Do the zero-writing in chunks
        while bytes_to_zero > 0 {
            let slice_len = core::cmp::min(self.rate - self.pos, bytes_to_zero);
            self.st.0[self.pos..(self.pos + slice_len)].copy_from_slice(&ZEROS[..slice_len]);

            self.pos += slice_len;
            bytes_to_zero -= slice_len;

            if self.pos == self.rate {
                self.run_f();
            }
        }
    }

    /// Mixes the current state index and flags into the state, accounting for whether we are
    /// sending or receiving
    fn begin_op(&mut self, mut flags: OpFlags) {
        if flags.contains(OpFlags::T) {
            let is_op_receiving = flags.contains(OpFlags::I);
            // If uninitialized, take on the direction of the first directional operation we get
            if self.is_receiver.is_none() {
                self.is_receiver = Some(is_op_receiving);
            }

            // So that the sender and receiver agree, toggle the I flag as necessary
            // This is equivalent to flags ^= is_receiver
            flags.set(OpFlags::I, self.is_receiver.unwrap() != is_op_receiving);
        }

        let old_pos_begin = self.pos_begin;
        self.pos_begin = self.pos + 1;

        // Mix in the position and flags
        let to_mix = &mut [old_pos_begin as u8, flags.bits()];
        self.absorb(&to_mix[..]);

        let force_f = flags.contains(OpFlags::C) || flags.contains(OpFlags::K);
        if force_f && self.pos != 0 {
            self.run_f();
        }
    }

    /// Performs the state / data transformation that corresponds to the given flags. If `more` is
    /// given, this will treat `data` as a continuation of the data given in the previous
    /// call to `operate`.
    pub(crate) fn operate(&mut self, flags: OpFlags, data: &mut [u8], more: bool) {
        // Make sure the K opflag isn't being used, and that the `more` flag isn't being misused
        assert!(!flags.contains(OpFlags::K), "Op flag K not implemented");
        self.validate_streaming(flags, more);

        // If `more` isn't set, this is a new operation. Do the begin_op sequence
        if !more {
            self.begin_op(flags);
        }

        // Meta-ness is only relevant for `begin_op`. Remove it to simplify the below logic.
        let flags = flags & !OpFlags::M;

        // TODO?: Assert that input is empty under some flag conditions
        if flags.contains(OpFlags::C) && flags.contains(OpFlags::T) && !flags.contains(OpFlags::I) {
            // This is equivalent to the `duplex` operation in the Python implementation, with
            // `cafter = True`
            if flags == OpFlags::C | OpFlags::T {
                // This is `send_mac`. Pretend the input is all zeros
                self.copy_state(data)
            } else {
                self.absorb_and_set(data);
            }
        } else if flags == OpFlags::I | OpFlags::A | OpFlags::C {
            // Special case of case below. This is PRF. Use `squeeze` instead of `exchange`.
            self.squeeze(data);
        } else if flags.contains(OpFlags::C) {
            // This is equivalent to the `duplex` operation in the Python implementation, with
            // `cbefore = True`
            self.exchange(data);
        } else {
            // This should normally call `absorb`, but `absorb` does not mutate, so the implementor
            // should have used operate_no_mutate instead
            panic!("operate should not be called for operations that do not require mutation");
        }
    }

    /// Performs the state transformation that corresponds to the given flags. If `more` is given,
    /// this will treat `data` as a continuation of the data given in the previous call to
    /// `operate`. This uses non-mutating variants of the specializations of the `duplex` function.
    pub(crate) fn operate_no_mutate(&mut self, flags: OpFlags, data: &[u8], more: bool) {
        // Make sure the K opflag isn't being used, and that the `more` flag isn't being misused
        assert!(!flags.contains(OpFlags::K), "Op flag K not implemented");
        self.validate_streaming(flags, more);

        // If `more` isn't set, this is a new operation. Do the begin_op sequence
        if !more {
            self.begin_op(flags);
        }

        // There are no non-mutating variants of things with flags & (C | T | I) == C | T
        if flags.contains(OpFlags::C) && flags.contains(OpFlags::T) && !flags.contains(OpFlags::I) {
            panic!("operate_no_mutate called on something that requires mutation");
        } else if flags.contains(OpFlags::C) {
            // This is equivalent to a non-mutating form of the `duplex` operation in the Python
            // implementation, with `cbefore = True`
            self.overwrite(data);
        } else {
            // This is equivalent to the `duplex` operation in the Python implementation, with
            // `cbefore = cafter = False`
            self.absorb(data);
        };
    }

    // This is separately defined because it's the only method that can return a `Result`. See docs
    // for recv_mac and meta_recv_mac.
    fn generalized_recv_mac(&mut self, data: &mut [u8], is_meta: bool) -> Result<(), AuthError> {
        // These are the (meta_)recv_mac flags
        let flags = if is_meta {
            OpFlags::I | OpFlags::C | OpFlags::T | OpFlags::M
        } else {
            OpFlags::I | OpFlags::C | OpFlags::T
        };
        // recv_mac can never be streamed
        self.operate(flags, data, /* more */ false);

        // Constant-time MAC check. This accumulates the truth values of byte == 0
        let mut all_zero = subtle::Choice::from(1u8);
        for b in data {
            all_zero &= b.ct_eq(&0u8);
        }

        // If the buffer isn't all zeros, that's an invalid MAC
        if !bool::from(all_zero) {
            Err(AuthError)
        } else {
            Ok(())
        }
    }

    /// Attempts to authenticate the current state against the given MAC. On failure, it returns an
    /// `AuthError`. It behooves the user of this library to check this return value and overreact
    /// on error.
    pub fn recv_mac(&mut self, data: &mut [u8]) -> Result<(), AuthError> {
        self.generalized_recv_mac(data, /* is_meta */ false)
    }

    /// Attempts to authenticate the current state against the given MAC. On failure, it returns an
    /// `AuthError`. It behooves the user of this library to check this return value and overreact
    /// on error.
    pub fn meta_recv_mac(&mut self, data: &mut [u8]) -> Result<(), AuthError> {
        self.generalized_recv_mac(data, /* is_meta */ true)
    }

    // This is separately defined because it's the only method that takes an integer and mutates
    // its input
    fn generalized_ratchet(&mut self, num_bytes_to_zero: usize, more: bool, is_meta: bool) {
        // These are the (meta_)ratchet flags
        let flags = if is_meta {
            OpFlags::C | OpFlags::M
        } else {
            OpFlags::C
        };

        // We don't make an `operate` call, since this is a super special case. That means we have
        // to validate the flags and make the `begin_op` call manually.
        self.validate_streaming(flags, more);
        if !more {
            self.begin_op(flags);
        }

        self.zero_state(num_bytes_to_zero);
    }

    /// Ratchets the internal state forward in an irreversible way by zeroing bytes.
    ///
    /// Takes a `usize` argument specifying the number of bytes of public state to zero. If the
    /// size exceeds `self.rate`, Keccak-f will be called before more bytes are zeroed.
    pub fn ratchet(&mut self, num_bytes_to_zero: usize, more: bool) {
        self.generalized_ratchet(num_bytes_to_zero, more, /* is_meta */ false)
    }

    /// Ratchets the internal state forward in an irreversible way by zeroing bytes.
    ///
    /// Takes a `usize` argument specifying the number of bytes of public state to zero. If the
    /// size exceeds `self.rate`, Keccak-f will be called before more bytes are zeroed.
    pub fn meta_ratchet(&mut self, num_bytes_to_zero: usize, more: bool) {
        self.generalized_ratchet(num_bytes_to_zero, more, /* is_meta */ true)
    }

    //
    // These operations mutate their inputs
    //

    def_op_mut!(
        send_enc,
        meta_send_enc,
        OpFlags::A | OpFlags::C | OpFlags::T,
        "Sends an encrypted message."
    );
    def_op_mut!(
        recv_enc,
        meta_recv_enc,
        OpFlags::I | OpFlags::A | OpFlags::C | OpFlags::T,
        "Receives an encrypted message."
    );
    def_op_mut!(
        send_mac,
        meta_send_mac,
        OpFlags::C | OpFlags::T,
        "Sends a MAC of the internal state. \
         The output is independent of the initial contents of the input buffer."
    );
    def_op_mut!(
        prf,
        meta_prf,
        OpFlags::I | OpFlags::A | OpFlags::C,
        "Extracts pseudorandom data as a function of the internal state. \
         The output is independent of the initial contents of the input buffer."
    );

    //
    // These operations do not mutate their inputs
    //

    def_op_no_mut!(
        send_clr,
        meta_send_clr,
        OpFlags::A | OpFlags::T,
        "Sends a plaintext message."
    );
    def_op_no_mut!(
        recv_clr,
        meta_recv_clr,
        OpFlags::I | OpFlags::A | OpFlags::T,
        "Receives a plaintext message."
    );
    def_op_no_mut!(
        ad,
        meta_ad,
        OpFlags::A,
        "Mixes associated data into the internal state."
    );
    def_op_no_mut!(
        key,
        meta_key,
        OpFlags::A | OpFlags::C,
        "Sets a symmetric cipher key."
    );
}

#[test]
fn version_str() {
    let s128 = Strobe::new(b"version_str test", SecParam::B128);
    assert_eq!(&s128.version_str(), b"Strobe-Keccak-128/1600-v1.0.2");

    let s256 = Strobe::new(b"version_str test", SecParam::B256);
    assert_eq!(&s256.version_str(), b"Strobe-Keccak-256/1600-v1.0.2");
}
