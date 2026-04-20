// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use ffi::CxxPolkadotDecodeUnsignedTransfer;
use parity_scale_codec::{Compact, Decode, Encode};
use std::fmt;

mod polkadot_chain_metadata;

const SIGNED_EXTRINSIC: u8 = 0x80;
const EXTRINSIC_VERSION: u8 = 0x04;
const MULTIADDRESS_TYPE: u8 = 0x00;
const SR25519_SIGNATURE: u8 = 0x01;
const PERIOD: u32 = 64;

const PHASE_APPLY_EXTRINSIC: u8 = 0;

const WITHDRAW_VARIANT_INDEX: u8 = 0x08;

// transactionpayment(TransactionFeePaid)
const TRANSACTION_FEE_PAID_VARIANT_INDEX: u8 = 0x00;

// system(ExtrinsicSuccess | ExtrinsicFailed)
const EXTRINSIC_SUCCESS_VARIANT_INDEX: u8 = 0x00;
const EXTRINSIC_FAILED_VARIANT_INDEX: u8 = 0x01;

// Default to reaping the account when invoking transfer_all. This means that we
// will transfer all available DOT instead of leaving the account with the
// existential deposit.
const TRANSFER_ALL_KEEP_ALIVE: bool = false;

const UNSIGNED_TRANSFER_ALLOW_DEATH_MIN_LEN: usize = 1  /* extrinsic version */
                                                   + 1  /* pallet index */
                                                   + 1  /* call index */
                                                   + 1  /* MultiAddress type */
                                                   + 32 /* recipient pubkey */
                                                   + 1  /* SCALE-encoded send amount */
                                                   ;

#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    #[derive(Clone, Copy)]
    pub struct CxxPolkadotDecodeUnsignedTransfer {
        pub recipient: [u8; 32],
        pub send_amount_bytes: [u8; 16],
    }

    #[derive(Clone, Copy, PartialEq)]
    pub struct CxxPolkadotChainMetadata {
        pub system_pallet_index: u8,
        pub balances_pallet_index: u8,
        pub transaction_payment_pallet_index: u8,
        pub transfer_allow_death_call_index: u8,
        pub transfer_keep_alive_call_index: u8,
        pub transfer_all_call_index: u8,
        pub ss58_prefix: u16,
        pub spec_version: u32,
    }

    extern "Rust" {
        fn compact_scale_encode_u32(x: u32) -> Vec<u8>;

        type CxxPolkadotDecodeUnsignedTransferResult;

        fn is_ok(self: &CxxPolkadotDecodeUnsignedTransferResult) -> bool;
        fn error_message(self: &CxxPolkadotDecodeUnsignedTransferResult) -> String;
        fn unwrap(
            self: &mut CxxPolkadotDecodeUnsignedTransferResult,
        ) -> Box<CxxPolkadotDecodeUnsignedTransfer>;

        fn encode_unsigned_transfer_allow_death(
            chain_metadata: &CxxPolkadotChainMetadata,
            send_amount_bytes: &[u8; 16],
            pubkey: &[u8; 32],
        ) -> Vec<u8>;

        fn decode_unsigned_transfer_allow_death(
            chain_metadata: &CxxPolkadotChainMetadata,
            mut input: &[u8],
        ) -> Box<CxxPolkadotDecodeUnsignedTransferResult>;

        fn scale_encode_mortality(number: u32, period: u32) -> [u8; 2];

        fn generate_extrinsic_signature_payload(
            chain_metadata: &CxxPolkadotChainMetadata,
            sender_nonce: u32,
            send_amount_bytes: &[u8; 16],
            transfer_all: bool,
            recipient: &[u8; 32],
            spec_version: u32,
            transaction_version: u32,
            block_number: u32,
            genesis_hash: &[u8; 32],
            block_hash: &[u8; 32],
        ) -> Vec<u8>;

        fn make_signed_extrinsic(
            chain_metadata: &CxxPolkadotChainMetadata,
            sender_pubkey: &[u8; 32],
            recipient_pubkey: &[u8; 32],
            send_amount_bytes: &[u8; 16],
            transfer_all: bool,
            signature: &[u8; 64],
            block_number: u32,
            sender_nonce: u32,
        ) -> Vec<u8>;

        fn parse_fee_info(input: &[u8], fee_bytes: &mut [u8; 16]) -> bool;

        fn was_extrinsic_successful(
            events: &[u8],
            extrinsic_idx: u32,
            sender: &[u8; 32],
            chain_metadata: &CxxPolkadotChainMetadata,
            actual_fee: &mut [u8; 16],
        ) -> bool;
    }
}

pub(crate) use ffi::CxxPolkadotChainMetadata;

#[macro_export]
macro_rules! impl_result {
    ($t:ident, $r:ident) => {
        struct $r(Result<$t, Error>);

        impl $r {
            fn error_message(self: &$r) -> String {
                match &self.0 {
                    Err(e) => e.to_string(),
                    Ok(_) => String::new(),
                }
            }

            fn is_ok(self: &$r) -> bool {
                self.0.is_ok()
            }

            fn unwrap(self: &mut $r) -> Box<$t> {
                match std::mem::replace(&mut self.0, Err(Error::AlreadyUnwrapped)) {
                    Ok(v) => Box::new(v),
                    Err(e) => panic!("{}", e.to_string()),
                }
            }
        }
    };
}

/// Errors that can occur when parsing Polkadot runtime metadata or parsing
/// extrinsics.
#[derive(Clone, Debug)]
pub enum Error {
    /// The Result has already been unwrapped.
    AlreadyUnwrapped,
    /// Invalid SCALE value found.
    InvalidScale,
    /// Invalid metadata such as the wrong pallet index or call index.
    InvalidMetadata,
    /// Invalid length.
    InvalidLength,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::AlreadyUnwrapped => write!(f, "Already unwrapped."),
            Error::InvalidScale => write!(f, "Invalid SCALE-encoded bytes were found."),
            Error::InvalidMetadata => write!(f, "Invalid chain metadata was encountered."),
            Error::InvalidLength => write!(f, "Invalid length."),
        }
    }
}

impl_result!(CxxPolkadotDecodeUnsignedTransfer, CxxPolkadotDecodeUnsignedTransferResult);

fn encode_unsigned_transfer_allow_death(
    chain_metadata: &CxxPolkadotChainMetadata,
    send_amount_bytes: &[u8; 16],
    pubkey: &[u8; 32],
) -> Vec<u8> {
    let CxxPolkadotChainMetadata { balances_pallet_index, transfer_allow_death_call_index, .. } =
        chain_metadata;

    let mut buf = Vec::<u8>::with_capacity(256);

    buf.extend_from_slice(&[
        EXTRINSIC_VERSION,
        *balances_pallet_index,
        *transfer_allow_death_call_index,
        MULTIADDRESS_TYPE,
    ]);

    buf.extend_from_slice(pubkey);

    Compact(u128::from_le_bytes(*send_amount_bytes)).encode_to(&mut buf);

    Compact(buf.len() as u64).using_encoded(|encoded_len| {
        buf.splice(0..0, encoded_len.iter().copied());
    });

    buf
}

fn next_n_bytes<'a>(input: &mut &'a [u8], n: usize) -> Result<&'a [u8], Error> {
    let Some((first, last)) = input.split_at_checked(n) else {
        return Err(Error::InvalidLength);
    };

    *input = last;
    Ok(first)
}

fn decode_unsigned_transfer_allow_death_impl(
    chain_metadata: &CxxPolkadotChainMetadata,
    mut input: &[u8],
) -> Result<CxxPolkadotDecodeUnsignedTransfer, Error> {
    let Ok(len) = Compact::<u64>::decode(&mut input) else {
        return Err(Error::InvalidScale);
    };

    let Ok(len) = usize::try_from(len.0) else {
        return Err(Error::InvalidLength);
    };

    if input.len() != len || len < UNSIGNED_TRANSFER_ALLOW_DEATH_MIN_LEN {
        return Err(Error::InvalidLength);
    }

    let CxxPolkadotChainMetadata { balances_pallet_index, transfer_allow_death_call_index, .. } =
        chain_metadata;

    if next_n_bytes(&mut input, 4)?
        != &[
            EXTRINSIC_VERSION,
            *balances_pallet_index,
            *transfer_allow_death_call_index,
            MULTIADDRESS_TYPE,
        ]
    {
        return Err(Error::InvalidMetadata);
    }

    let mut recipient = [0_u8; 32];
    recipient.copy_from_slice(next_n_bytes(&mut input, 32)?);

    debug_assert!(!input.is_empty());

    let Ok(send_amount) = Compact::<u128>::decode(&mut input) else {
        return Err(Error::InvalidScale);
    };

    let send_amount = send_amount.0;
    let mut send_amount_bytes = [0_u8; 16];
    send_amount_bytes.copy_from_slice(&send_amount.to_le_bytes());

    Ok(CxxPolkadotDecodeUnsignedTransfer { recipient, send_amount_bytes })
}

fn decode_unsigned_transfer_allow_death(
    chain_metadata: &CxxPolkadotChainMetadata,
    input: &[u8],
) -> Box<CxxPolkadotDecodeUnsignedTransferResult> {
    let r = decode_unsigned_transfer_allow_death_impl(chain_metadata, input);
    Box::new(CxxPolkadotDecodeUnsignedTransferResult(r))
}

/// Mortality tells the blockchain what range of blocks an extrinsic is valid
/// for. Validators use the period and phase to find the block hash included in
/// the payload, which they use to determine signature validity (and thus
/// transaction validity).
///
/// We use a period of 64, as send thransactions should be of "high priority"
/// and thus subject to smaller mortality windows, i.e. we don't want an
/// extrinsic sitting in the memory pool while the world outside it continues to
/// change.
///
/// For information on how mortality is encoded, see the documentation at:
/// https://spec.polkadot.network/id-extrinsics#id-mortality
///
/// Reference implementation:
/// https://github.com/polkadot-js/api/blob/9f6a9c53e6822d20e8556649c9b68d31cffc465d/packages/types/src/extrinsic/ExtrinsicEra.ts#L179-L204
fn scale_encode_mortality(number: u32, mut period: u32) -> [u8; 2] {
    period = period.checked_next_power_of_two().unwrap().clamp(4, 1 << 16);

    let phase = number % period;
    let factor = (period >> 12).max(1);

    let left = period.trailing_zeros().saturating_sub(1).clamp(1, 15);
    let right = (phase / factor) << 4;
    let encoded = u16::try_from(left | right).unwrap();

    [(encoded & 0xff) as u8, (encoded >> 8) as u8]
}

/// The definition for the extrinsic signature can be found here:
/// https://spec.polkadot.network/id-extrinsics#defn-extrinsic-signature
///
/// Extrinsic signatures are created using the following data:
/// P = { Raw if ||Raw|| <= 256, Blake2(Raw) if ||Raw|| > 256 }
/// Raw = (M_i, F_i(m), E, R_v, F_v, H_h(G), H_h(B))
///
/// M_i = module indicator of the extrinsic (i.e. System, Balances, ...).
/// F_i(m) = function indicator of the module (i.e. call index, parameters).
/// E = extra data (mortality, nonce of sender, tip, mode).
/// R_v = spec_version of the runtime (fetched via state_getRuntimeVersion).
/// F_v = transaction version of the runtime (fetched via
///       state_getRuntimeVersion).
/// H_h(G) = block hash of the genesis block.
/// H_h(B) = block hash of the block which starts the extrinsic's mortality.
///
/// We return a binary blob containing all of these fields so that it's suitable
/// for cryptographic signing.
///
/// The formal spec doesn't define it but here:
/// https://wiki.polkadot.com/learn/learn-transaction-construction/
/// we learn that metadata hashing can be applied to extrinsics but we disable
/// this verification by setting the mode to 0.
/// See also:
/// https://github.com/polkadot-fellows/RFCs/blob/85ca3ff275ded2e690c4c175d5333d12b139d863/text/0078-merkleized-metadata.md
fn generate_extrinsic_signature_payload(
    chain_metadata: &CxxPolkadotChainMetadata,
    sender_nonce: u32,
    send_amount_bytes: &[u8; 16],
    transfer_all: bool,
    recipient: &[u8; 32],
    spec_version: u32,
    transaction_version: u32,
    block_number: u32,
    genesis_hash: &[u8; 32],
    block_hash: &[u8; 32],
) -> Vec<u8> {
    let CxxPolkadotChainMetadata {
        balances_pallet_index,
        transfer_keep_alive_call_index,
        transfer_all_call_index,
        ..
    } = chain_metadata;

    let mut buf = Vec::<u8>::with_capacity(256);

    let call_idx =
        if transfer_all { transfer_all_call_index } else { transfer_keep_alive_call_index };

    buf.extend_from_slice(&[
        /* Write module indicator, M_i. */
        *balances_pallet_index,
        /* Write function indicator (call index + call parameters). */
        *call_idx,
        MULTIADDRESS_TYPE,
    ]);

    buf.extend_from_slice(recipient);
    if transfer_all {
        TRANSFER_ALL_KEEP_ALIVE.encode_to(&mut buf);
    } else {
        Compact(u128::from_le_bytes(*send_amount_bytes)).encode_to(&mut buf);
    }

    // Write extra data, E.
    buf.extend_from_slice(&scale_encode_mortality(block_number, PERIOD));
    Compact(sender_nonce).encode_to(&mut buf);
    buf.extend_from_slice(&[
        0x00, /* tip */
        0x00, /* mode (disable metadata hash verification) */
    ]);

    // Write R_v.
    buf.extend_from_slice(&spec_version.to_le_bytes());

    // Write F_v.
    buf.extend_from_slice(&transaction_version.to_le_bytes());

    // Write H_h(G).
    buf.extend_from_slice(genesis_hash);

    // Write H_h(B)
    buf.extend_from_slice(block_hash);

    // Write metadata hash (nullary).
    buf.extend_from_slice(&[0x00]);

    // If our payload exceeds 256 bytes, hash it down to 32 bytes as demonstarted by
    // the polkadot-js implementation here:
    // https://github.com/polkadot-js/api/blob/9f6a9c53e6822d20e8556649c9b68d31cffc465d/packages/types/src/extrinsic/util.ts#L10-L17
    // https://github.com/polkadot-js/common/blob/bf63a0ebf655312f54aa37350d244df3d05e4e32/packages/util-crypto/src/blake2/asU8a.ts#L25-L34
    if buf.len() > 256 {
        let hash = blake2b_simd::Params::new().hash_length(32).hash(&buf);
        buf.clear();
        buf.extend_from_slice(hash.as_bytes());
    }

    assert!(buf.len() <= 256);
    buf
}

fn make_signed_extrinsic(
    chain_metadata: &CxxPolkadotChainMetadata,
    sender_pubkey: &[u8; 32],
    recipient_pubkey: &[u8; 32],
    send_amount_bytes: &[u8; 16],
    transfer_all: bool,
    signature: &[u8; 64],
    block_number: u32,
    sender_nonce: u32,
) -> Vec<u8> {
    let CxxPolkadotChainMetadata {
        balances_pallet_index,
        transfer_keep_alive_call_index,
        transfer_all_call_index,
        ..
    } = chain_metadata;

    let mut buf = Vec::<u8>::with_capacity(512);

    buf.extend_from_slice(&[SIGNED_EXTRINSIC | EXTRINSIC_VERSION, MULTIADDRESS_TYPE]);
    buf.extend_from_slice(sender_pubkey);
    buf.extend_from_slice(&[SR25519_SIGNATURE]);
    buf.extend_from_slice(signature);
    buf.extend_from_slice(&scale_encode_mortality(block_number, PERIOD));
    Compact(sender_nonce).encode_to(&mut buf);
    buf.extend_from_slice(&[0x00 /* tip */, 0x00 /* mode */]);

    let call_idx =
        if transfer_all { transfer_all_call_index } else { transfer_keep_alive_call_index };

    buf.extend_from_slice(&[*balances_pallet_index, *call_idx, MULTIADDRESS_TYPE]);
    buf.extend_from_slice(recipient_pubkey);

    if transfer_all {
        TRANSFER_ALL_KEEP_ALIVE.encode_to(&mut buf);
    } else {
        Compact(u128::from_le_bytes(*send_amount_bytes)).encode_to(&mut buf);
    }

    Compact(buf.len() as u64).using_encoded(|encoded_len| {
        buf.splice(0..0, encoded_len.iter().copied());
    });

    buf
}

// Definition of the type's binary representation is provided here:
// https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/types-support/src/metadata/v15/polkadot-types.json#L66282-L66305
//
// The general shape of the octets sent from the RPC nodes is:
// {weight, class, partial_fee}
// weight = {ref_time (as Compact<u64>), proof_size (as Compact<u64>)}
// class = 0x00, 0x01, or 0x02
// partial_fee = LE bytes representing U128
fn parse_fee_info(input: &[u8], fee_bytes: &mut [u8; 16]) -> bool {
    // Normally in C++, a reference is an immutable thing that once it's bound to an
    // object, it can never re-alias. In Rust, a reference _is_ a pointer. The
    // parity-scale-codec crate takes advantage of this and its API mutates the
    // input pointer, advancing it as it parses.
    let mut input = input;

    let ref_time = <Compact<u64>>::decode(&mut input);
    if ref_time.is_err() {
        return false;
    }

    let proof_size = <Compact<u64>>::decode(&mut input);
    if proof_size.is_err() {
        return false;
    }

    const CLASS_NORMAL: u8 = 0;
    const CLASS_OPERATIONAL: u8 = 1;
    const CLASS_MANDATORY: u8 = 2;

    let Ok(class) = next_n_bytes(&mut input, 1) else { return false };
    match class[0] {
        // These are the only valid values
        // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/types-support/src/metadata/v15/polkadot-types.json#L1330-L1349
        CLASS_NORMAL | CLASS_OPERATIONAL | CLASS_MANDATORY => {}
        _ => return false,
    }

    let Ok(fee_le_bytes) = next_n_bytes(&mut input, 16) else { return false };
    if !input.is_empty() {
        // Trailing octets, assume invalid input.
        return false;
    }

    fee_bytes.copy_from_slice(fee_le_bytes);

    true
}

fn compact_scale_encode_u32(x: u32) -> Vec<u8> {
    Compact(x).encode()
}

fn was_extrinsic_successful(
    events: &[u8],
    extrinsic_idx: u32,
    sender: &[u8; 32],
    chain_metadata: &CxxPolkadotChainMetadata,
    actual_fee: &mut [u8; 16],
) -> bool {
    /*
        For a send transaction, a simplified event flow looks roughly like this:

            ┌─────────────────────────────────┐
            │        balances(Withdraw)       │
            └─────────────────────────────────┘
                      │              │
                [success]          [error]
                      │              │
                      ▼              │
            ┌──────────────────────┐ │
            │  balances(Transfer)  │ │
            └──────────────────────┘ │
                      │              │
                      └──────┬───────┘
                             │
                             ▼
            ┌───────────────────────────────────┐
            │        balances(Deposit), ...     │
            └───────────────────────────────────┘
                             │
                             ▼
            ┌──────────────────────────────────────────┐
            │ transactionpayment(TransactionFeePaid)   │
            └──────────────────────────────────────────┘
                             │
                    ┌────────┴──────────────────────┐
                    │                               │
                    ▼                               ▼
            ┌──────────────────────────┐ ┌─────────────────────────┐
            │ system(ExtrinsicSuccess) │ │ system(ExtrinsicFailed) │
            └──────────────────────────┘ └─────────────────────────┘
    */

    // But in general, it seems like the events flow can become quite complex:
    // https://polkadot.subscan.io/extrinsic/30123219-2
    // The thing to note is that the extrinsic always ends with the same two
    // events, the fee was paid and the system gave the extrinsic a final status.
    //
    // In Polkadot, an event is defined as: {phase, event, topics}
    // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/types-support/src/metadata/v15/polkadot-types.json#L519-L542
    //
    // Because the events are a massive binary blob that rely on quite a bit of
    // Polkadot runtime metadata to fully parse, we just probe for the two events
    // for our extrinsic that we care about: the transaction fee paid and the final
    // status. We can theoretically probe for everything such as who the fee was
    // paid out to but it isn't strictly required for our current needs.

    // We first probe for the balances(Withdraw) event, so that we can use the
    // withdrawn fee as a sanity check when we probe for our TransactionFeePaid
    // event later on.
    let mut withdraw_needle = [0_u8; 39];
    withdraw_needle[0] = PHASE_APPLY_EXTRINSIC;
    withdraw_needle[1..5].copy_from_slice(&extrinsic_idx.to_le_bytes());
    withdraw_needle[5] = chain_metadata.balances_pallet_index;
    withdraw_needle[6] = WITHDRAW_VARIANT_INDEX;
    withdraw_needle[7..39].copy_from_slice(sender);

    // Use `rfind` here because extrinsic blobs can be huge, and our events are
    // typically found at the end of the events blob.
    let mut events = events;
    let Some(needle_idx) = memchr::memmem::rfind(events, &withdraw_needle) else {
        return false;
    };

    events = &events[needle_idx + withdraw_needle.len()..];

    let Ok(withdrawn_fee) = next_n_bytes(&mut events, 16) else {
        return false;
    };

    let Ok(topics) = next_n_bytes(&mut events, 1) else {
        return false;
    };

    if topics[0] != 0 {
        return false;
    };

    // Look for the remainining two events we need,
    // transactionpayment(TransactionFeePaid) and system(ExtrinsicSuccess |
    // ExtrinsicFailed)
    let mut transaction_fee_paid_needle = [0_u8; 39];
    transaction_fee_paid_needle[0] = PHASE_APPLY_EXTRINSIC;
    transaction_fee_paid_needle[1..5].copy_from_slice(&extrinsic_idx.to_le_bytes());
    transaction_fee_paid_needle[5] = chain_metadata.transaction_payment_pallet_index;
    transaction_fee_paid_needle[6] = TRANSACTION_FEE_PAID_VARIANT_INDEX;
    transaction_fee_paid_needle[7..39].copy_from_slice(sender);

    // Use `find` here because we've located the start of our event sequence above.
    let Some(needle_idx) = memchr::memmem::find(events, &transaction_fee_paid_needle) else {
        return false;
    };

    events = &events[needle_idx + transaction_fee_paid_needle.len()..];
    let Ok(fee) = next_n_bytes(&mut events, 16) else {
        return false;
    };

    // If our fees don't match here, we can consider the events blob invalid.
    if withdrawn_fee != fee {
        return false;
    }

    let Ok(_tip) = next_n_bytes(&mut events, 16) else {
        return false;
    };

    let Ok(topics) = next_n_bytes(&mut events, 1) else {
        return false;
    };

    if topics[0] != 0 {
        return false;
    };

    let Ok(phase) = next_n_bytes(&mut events, 1) else {
        return false;
    };

    if phase[0] != PHASE_APPLY_EXTRINSIC {
        return false;
    }

    let Ok(idx) = next_n_bytes(&mut events, 4) else {
        return false;
    };

    if idx != &extrinsic_idx.to_le_bytes() {
        return false;
    }

    let Ok(call_index) = next_n_bytes(&mut events, 2) else {
        return false;
    };

    if call_index[0] != chain_metadata.system_pallet_index {
        return false;
    };

    if call_index[1] != EXTRINSIC_SUCCESS_VARIANT_INDEX
        && call_index[1] != EXTRINSIC_FAILED_VARIANT_INDEX
    {
        return false;
    };

    actual_fee.copy_from_slice(fee);
    call_index[1] == EXTRINSIC_SUCCESS_VARIANT_INDEX
}
