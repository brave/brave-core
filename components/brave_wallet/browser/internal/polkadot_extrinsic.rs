// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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

#[cxx::bridge(namespace = brave_wallet)]
mod ffi {
    #[derive(Clone, Copy, PartialEq)]
    pub struct CxxPolkadotChainMetadata {
        pub system_pallet_index: u8,
        pub balances_pallet_index: u8,
        pub transaction_payment_pallet_index: u8,
        pub transfer_allow_death_call_index: u8,
        pub transfer_keep_alive_call_index: u8,
        pub transfer_all_call_index: u8,
        pub assets_pallet_index: u8,
        pub assets_transfer_all_call_index: u8,
        pub assets_transfer_keep_alive_call_index: u8,
        pub has_assets_pallet: bool,
        pub ss58_prefix: u16,
        pub spec_version: u32,
        pub signed_extensions: [u8; 64],
    }

    /// Holds bytes used for signature payloads and signed extrinsics.
    pub struct CxxPolkadotExtrinsic {
        pub bytes: Vec<u8>,
    }

    extern "Rust" {
        fn compact_scale_encode_u32(x: u32) -> Vec<u8>;
        fn scale_encode_string(value: &[u8]) -> Vec<u8>;

        type CxxPolkadotChainMetadataResult;

        fn is_ok(self: &CxxPolkadotChainMetadataResult) -> bool;
        fn error_message(self: &CxxPolkadotChainMetadataResult) -> String;
        fn unwrap(self: &mut CxxPolkadotChainMetadataResult) -> Box<CxxPolkadotChainMetadata>;

        fn parse_chain_metadata_from_scale(
            metadata_bytes: &[u8],
        ) -> Box<CxxPolkadotChainMetadataResult>;

        type CxxPolkadotExtrinsicResult;

        fn is_ok(self: &CxxPolkadotExtrinsicResult) -> bool;
        fn error_message(self: &CxxPolkadotExtrinsicResult) -> String;
        fn unwrap(self: &mut CxxPolkadotExtrinsicResult) -> Box<CxxPolkadotExtrinsic>;

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
        ) -> Box<CxxPolkadotExtrinsicResult>;

        fn generate_assets_extrinsic_signature_payload(
            chain_metadata: &CxxPolkadotChainMetadata,
            sender_nonce: u32,
            send_amount_bytes: &[u8; 16],
            transfer_all: bool,
            recipient: &[u8; 32],
            asset_id: u32,
            spec_version: u32,
            transaction_version: u32,
            block_number: u32,
            genesis_hash: &[u8; 32],
            block_hash: &[u8; 32],
        ) -> Box<CxxPolkadotExtrinsicResult>;

        fn make_signed_extrinsic(
            chain_metadata: &CxxPolkadotChainMetadata,
            sender_pubkey: &[u8; 32],
            recipient_pubkey: &[u8; 32],
            send_amount_bytes: &[u8; 16],
            transfer_all: bool,
            signature: &[u8; 64],
            block_number: u32,
            sender_nonce: u32,
        ) -> Box<CxxPolkadotExtrinsicResult>;

        fn make_signed_asset_transfer_extrinsic(
            chain_metadata: &CxxPolkadotChainMetadata,
            sender_pubkey: &[u8; 32],
            recipient_pubkey: &[u8; 32],
            send_amount_bytes: &[u8; 16],
            transfer_all: bool,
            signature: &[u8; 64],
            block_number: u32,
            sender_nonce: u32,
            asset_id: u32,
        ) -> Box<CxxPolkadotExtrinsicResult>;

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

use ffi::CxxPolkadotChainMetadata;
impl_result!(CxxPolkadotChainMetadata, CxxPolkadotChainMetadataResult);

use ffi::CxxPolkadotExtrinsic;
impl_result!(CxxPolkadotExtrinsic, CxxPolkadotExtrinsicResult);

use crate::polkadot_chain_metadata::parse_chain_metadata_from_scale;

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
    /// Unknowned SignedExtension
    UnknownSignedExtension,
    /// SignedExtension overflow (more than we've anticipated).
    SignedExtensionOverflow,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::AlreadyUnwrapped => write!(f, "Already unwrapped."),
            Error::InvalidScale => write!(f, "Invalid SCALE-encoded bytes were found."),
            Error::InvalidMetadata => write!(f, "Invalid chain metadata was encountered."),
            Error::InvalidLength => write!(f, "Invalid length."),
            Error::UnknownSignedExtension => write!(f, "Unknowned SignedExtension encountered."),
            Error::SignedExtensionOverflow => {
                write!(f, "SignedExtension overflow was encountered.")
            }
        }
    }
}

fn next_n_bytes<'a>(input: &mut &'a [u8], n: usize) -> Result<&'a [u8], Error> {
    let Some((first, last)) = input.split_at_checked(n) else {
        return Err(Error::InvalidLength);
    };

    *input = last;
    Ok(first)
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

fn append_extra(
    chain_metadata: &CxxPolkadotChainMetadata,
    sender_nonce: u32,
    block_number: u32,
    buf: &mut Vec<u8>,
) -> Result<(), Error> {
    use polkadot_chain_metadata::SignedExtension;

    for &extension in &chain_metadata.signed_extensions {
        if extension == 0 {
            continue;
        }

        let Ok(extension) = extension.try_into() else {
            return Err(Error::UnknownSignedExtension);
        };

        use parity_scale_codec::Encode;

        // None encodes to the same underlying 0x00 value, no matter what the value of
        // Option<T> is, so None::<()> is sufficient for our purposes.

        match extension {
            SignedExtension::AuthorizeValueTransfer
            | SignedExtension::AsPgas
            | SignedExtension::AsRingAlias
            | SignedExtension::AsDotnsGateway => {
                None::<()>.encode_to(buf);
            }
            SignedExtension::RestrictOrigins => {
                false.encode_to(buf);
            }
            SignedExtension::CheckMortality => {
                buf.extend_from_slice(&scale_encode_mortality(block_number, PERIOD));
            }
            SignedExtension::CheckNonce => {
                Compact(sender_nonce).encode_to(buf);
            }
            SignedExtension::ChargeAssetTxPayment => {
                Compact(0x00_u128).encode_to(buf); /* tip */
                None::<()>.encode_to(buf) /* asset_id */
            }
            SignedExtension::ChargeTransactionPayment => {
                Compact(0x00_u128).encode_to(buf); /* tip */
            }
            SignedExtension::CheckMetadataHash => {
                // Disabled is the 0'th variant of the Mode type.
                buf.extend_from_slice(&[0x00 /* mode */]);
            }
            _ => continue,
        }
    }

    Ok(())
}

fn append_implicit(
    chain_metadata: &CxxPolkadotChainMetadata,
    spec_version: u32,
    transaction_version: u32,
    genesis_hash: &[u8; 32],
    block_hash: &[u8; 32],
    buf: &mut Vec<u8>,
) -> Result<(), Error> {
    use polkadot_chain_metadata::SignedExtension;

    for &extension in &chain_metadata.signed_extensions {
        if extension == 0 {
            continue;
        }

        let Ok(extension) = extension.try_into() else {
            return Err(Error::UnknownSignedExtension);
        };

        match extension {
            SignedExtension::CheckSpecVersion => {
                buf.extend_from_slice(&spec_version.to_le_bytes());
            }
            SignedExtension::CheckTxVersion => {
                buf.extend_from_slice(&transaction_version.to_le_bytes());
            }
            SignedExtension::CheckGenesis => {
                buf.extend_from_slice(genesis_hash);
            }
            SignedExtension::CheckMortality => {
                buf.extend_from_slice(block_hash);
            }
            SignedExtension::CheckMetadataHash => {
                // Disabled is the 0'th variant of the Mode type.
                buf.extend_from_slice(&[0x00 /* mode */]);
            }
            _ => continue,
        }
    }

    Ok(())
}

// To generate the signature payload, we follow what's in the polkadot-sdk docs.
// We create a binary string by concatenating the call data, and then the
// transaction extensions. Transaction extensions comes in two varities and the
// same name will have two different types, based on the context.
// We manually curate a whitelist of the associated types for each extension and
// write it out appropriately.
// An example of this dual-type nature can be seen here:
// https://github.com/polkadot-js/api/blob/db81417393858fa40b7b8168d80d52280db407d2/packages/types-support/src/metadata/v16/asset-hub-polkadot-json.json#L11468-L11535
//
// It's important to note that on-chain metadata doesn't necessarily match
// what's in the above link.
// https://paritytech.github.io/polkadot-sdk/master/polkadot_sdk_docs/reference_docs/extrinsic_encoding/index.html#the-signed-payload-format
fn generate_extrinsic_signature_payload_impl(
    chain_metadata: &CxxPolkadotChainMetadata,
    sender_nonce: u32,
    send_amount_bytes: &[u8; 16],
    transfer_all: bool,
    recipient: &[u8; 32],
    asset_id: Option<u32>,
    spec_version: u32,
    transaction_version: u32,
    block_number: u32,
    genesis_hash: &[u8; 32],
    block_hash: &[u8; 32],
) -> Result<Vec<u8>, Error> {
    let &CxxPolkadotChainMetadata {
        balances_pallet_index,
        transfer_keep_alive_call_index,
        transfer_all_call_index,
        has_assets_pallet,
        assets_pallet_index,
        assets_transfer_keep_alive_call_index,
        assets_transfer_all_call_index,
        ..
    } = chain_metadata;

    let mut buf = Vec::<u8>::with_capacity(256);

    let (pallet_idx, call_idx) = {
        if asset_id.is_some() {
            assert!(has_assets_pallet);

            let transfer_call_idx = if transfer_all {
                assets_transfer_all_call_index
            } else {
                assets_transfer_keep_alive_call_index
            };

            (assets_pallet_index, transfer_call_idx)
        } else {
            let transfer_call_idx =
                if transfer_all { transfer_all_call_index } else { transfer_keep_alive_call_index };

            (balances_pallet_index, transfer_call_idx)
        }
    };

    buf.extend_from_slice(&[
        pallet_idx, // Write module indicator, M_i.
        call_idx,   // Write function indicator (call index + call parameters).
    ]);

    if let Some(asset_id) = asset_id {
        Compact(asset_id).encode_to(&mut buf);
    }

    buf.extend_from_slice(&[MULTIADDRESS_TYPE]);
    buf.extend_from_slice(recipient);
    if transfer_all {
        TRANSFER_ALL_KEEP_ALIVE.encode_to(&mut buf);
    } else {
        Compact(u128::from_le_bytes(*send_amount_bytes)).encode_to(&mut buf);
    }

    // https://paritytech.github.io/polkadot-sdk/master/polkadot_sdk_docs/reference_docs/extrinsic_encoding/index.html#the-signed-payload-format
    append_extra(chain_metadata, sender_nonce, block_number, &mut buf)?;
    append_implicit(
        chain_metadata,
        spec_version,
        transaction_version,
        genesis_hash,
        block_hash,
        &mut buf,
    )?;

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
    Ok(buf)
}

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
) -> Box<CxxPolkadotExtrinsicResult> {
    let result = generate_extrinsic_signature_payload_impl(
        chain_metadata,
        sender_nonce,
        send_amount_bytes,
        transfer_all,
        recipient,
        None,
        spec_version,
        transaction_version,
        block_number,
        genesis_hash,
        block_hash,
    )
    .map(|bytes| CxxPolkadotExtrinsic { bytes });

    Box::new(CxxPolkadotExtrinsicResult(result))
}

fn generate_assets_extrinsic_signature_payload(
    chain_metadata: &CxxPolkadotChainMetadata,
    sender_nonce: u32,
    send_amount_bytes: &[u8; 16],
    transfer_all: bool,
    recipient: &[u8; 32],
    asset_id: u32,
    spec_version: u32,
    transaction_version: u32,
    block_number: u32,
    genesis_hash: &[u8; 32],
    block_hash: &[u8; 32],
) -> Box<CxxPolkadotExtrinsicResult> {
    let result = generate_extrinsic_signature_payload_impl(
        chain_metadata,
        sender_nonce,
        send_amount_bytes,
        transfer_all,
        recipient,
        Some(asset_id),
        spec_version,
        transaction_version,
        block_number,
        genesis_hash,
        block_hash,
    )
    .map(|bytes| CxxPolkadotExtrinsic { bytes });

    Box::new(CxxPolkadotExtrinsicResult(result))
}

fn make_signed_transfer_extrinsic_impl(
    chain_metadata: &CxxPolkadotChainMetadata,
    sender_pubkey: &[u8; 32],
    recipient_pubkey: &[u8; 32],
    send_amount_bytes: &[u8; 16],
    transfer_all: bool,
    signature: &[u8; 64],
    block_number: u32,
    sender_nonce: u32,
    asset_id: Option<u32>,
) -> Result<Vec<u8>, Error> {
    let &CxxPolkadotChainMetadata {
        balances_pallet_index,
        transfer_keep_alive_call_index,
        transfer_all_call_index,
        has_assets_pallet,
        assets_pallet_index,
        assets_transfer_keep_alive_call_index,
        assets_transfer_all_call_index,
        ..
    } = chain_metadata;

    let mut buf = Vec::<u8>::with_capacity(512);

    buf.extend_from_slice(&[SIGNED_EXTRINSIC | EXTRINSIC_VERSION, MULTIADDRESS_TYPE]);
    buf.extend_from_slice(sender_pubkey);
    buf.extend_from_slice(&[SR25519_SIGNATURE]);
    buf.extend_from_slice(signature);
    append_extra(chain_metadata, sender_nonce, block_number, &mut buf)?;

    let (pallet_idx, call_idx) = {
        if asset_id.is_some() {
            assert!(has_assets_pallet);

            let transfer_call_idx = if transfer_all {
                assets_transfer_all_call_index
            } else {
                assets_transfer_keep_alive_call_index
            };

            (assets_pallet_index, transfer_call_idx)
        } else {
            let transfer_call_idx =
                if transfer_all { transfer_all_call_index } else { transfer_keep_alive_call_index };

            (balances_pallet_index, transfer_call_idx)
        }
    };

    buf.extend_from_slice(&[pallet_idx, call_idx]);
    if let Some(asset_id) = asset_id {
        Compact(asset_id).encode_to(&mut buf);
    }

    buf.extend_from_slice(&[MULTIADDRESS_TYPE]);
    buf.extend_from_slice(recipient_pubkey);

    if transfer_all {
        TRANSFER_ALL_KEEP_ALIVE.encode_to(&mut buf);
    } else {
        Compact(u128::from_le_bytes(*send_amount_bytes)).encode_to(&mut buf);
    }

    Compact(buf.len() as u64).using_encoded(|encoded_len| {
        buf.splice(0..0, encoded_len.iter().copied());
    });

    Ok(buf)
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
) -> Box<CxxPolkadotExtrinsicResult> {
    let result = make_signed_transfer_extrinsic_impl(
        chain_metadata,
        sender_pubkey,
        recipient_pubkey,
        send_amount_bytes,
        transfer_all,
        signature,
        block_number,
        sender_nonce,
        None,
    )
    .map(|bytes| CxxPolkadotExtrinsic { bytes });

    Box::new(CxxPolkadotExtrinsicResult(result))
}

fn make_signed_asset_transfer_extrinsic(
    chain_metadata: &CxxPolkadotChainMetadata,
    sender_pubkey: &[u8; 32],
    recipient_pubkey: &[u8; 32],
    send_amount_bytes: &[u8; 16],
    transfer_all: bool,
    signature: &[u8; 64],
    block_number: u32,
    sender_nonce: u32,
    asset_id: u32,
) -> Box<CxxPolkadotExtrinsicResult> {
    let result = make_signed_transfer_extrinsic_impl(
        chain_metadata,
        sender_pubkey,
        recipient_pubkey,
        send_amount_bytes,
        transfer_all,
        signature,
        block_number,
        sender_nonce,
        Some(asset_id),
    )
    .map(|bytes| CxxPolkadotExtrinsic { bytes });

    Box::new(CxxPolkadotExtrinsicResult(result))
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

fn scale_encode_string(value: &[u8]) -> Vec<u8> {
    value.encode()
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
