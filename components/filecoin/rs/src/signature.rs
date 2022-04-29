use crate::message::MessageAPI;
use blake2b_simd::Params;
use bls_signatures::Serialize;
use core::{array::TryFromSliceError, num::ParseIntError};
use fvm_ipld_encoding::Cbor;
use fvm_shared::address::Protocol;
use fvm_shared::message::Message as UnsignedMessage;
use libsecp256k1::util::{SECRET_KEY_SIZE, SIGNATURE_SIZE};
use libsecp256k1::{sign, Message};
use std::convert::TryInto;
use thiserror::Error;

pub const SIGNATURE_RECOVERY_SIZE: usize = SIGNATURE_SIZE + 1;
pub const BLS_SIGNATURE_SIZE: usize = 96;

pub struct PrivateKey(pub [u8; SECRET_KEY_SIZE]);
pub struct SignatureBLS(pub [u8; BLS_SIGNATURE_SIZE]);
pub struct SignatureSECP256K1(pub [u8; SIGNATURE_RECOVERY_SIZE]);

pub enum Signature {
    SignatureSECP256K1(SignatureSECP256K1),
    SignatureBLS(SignatureBLS),
}

impl Signature {
    pub fn as_bytes(&self) -> &[u8] {
        match self {
            Signature::SignatureSECP256K1(sig_secp256k1) => sig_secp256k1.as_bytes(),
            Signature::SignatureBLS(sig_bls) => sig_bls.as_bytes(),
        }
    }
}

impl SignatureBLS {
    pub fn as_bytes(&self) -> &[u8] {
        &self.0
    }
}

impl SignatureSECP256K1 {
    pub fn as_bytes(&self) -> &[u8] {
        &self.0
    }
}

/// Filecoin Signer Error
#[derive(Error, Debug)]
pub enum SignerError {
    ///  CBOR error
    #[error("CBOR error: '{0}'")]
    CBOR(#[from] serde_cbor::Error),
    /// Secp256k1 error
    #[error("secp256k1 error")]
    Secp256k1(#[from] libsecp256k1::Error),
    // Key decoding error
    #[error("key decoding error (only hex or base64 is accepted)")]
    KeyDecoding(),
    /// Hex Error
    #[error("Hex decoding error | {0}")]
    HexDecode(#[from] hex::FromHexError),
    /// InvalidBigInt error
    #[error("InvalidBigInt error")]
    InvalidBigInt(#[from] num_bigint_chainsafe::ParseBigIntError),
    /// Generic error message
    #[error("Error: `{0}`")]
    GenericString(String),
    /// Not able to parse integer
    #[error("Cannot parse integer")]
    ParseIntError(#[from] ParseIntError),
    /// BLS error
    #[error("BLS error | {0}")]
    BLS(#[from] bls_signatures::Error),
    /// Invalid BIP44Path
    #[error("Invalid BIP44 path : `{0}`")]
    InvalidBIP44Path(#[from] zx_bip44::errors::BIP44PathError),
    /// BLS error
    #[error("Couldn't convert from slice")]
    TryFromSlice(#[from] TryFromSliceError),
    /// Base64 decode Error
    #[error("Base64 decode error | {0}")]
    DecodeError(#[from] base64::DecodeError),

    #[error("Marshall error | {0}")]
    FvmSharedEncodingError(#[from] fvm_ipld_encoding::Error),
}

const CID_PREFIX: &[u8] = &[0x01, 0x71, 0xa0, 0xe4, 0x02, 0x20];
const HASH_LENGTH: usize = 32;

/// transform a message into a hashed message ready to be signed and following Filecoin standard
pub fn get_digest(message: &[u8]) -> Result<[u8; HASH_LENGTH], TryFromSliceError> {
    let message_hashed =
        Params::new().hash_length(HASH_LENGTH).to_state().update(message).finalize();

    let cid_hashed = Params::new()
        .hash_length(HASH_LENGTH)
        .to_state()
        .update(CID_PREFIX)
        .update(message_hashed.as_bytes())
        .finalize();

    cid_hashed.as_bytes().try_into()
}

fn transaction_sign_secp56k1_raw(
    unsigned_message_api: &UnsignedMessage,
    private_key: &PrivateKey,
) -> Result<SignatureSECP256K1, SignerError> {
    let message_cbor = unsigned_message_api.marshal_cbor()?;

    let secret_key = libsecp256k1::SecretKey::parse_slice(&private_key.0)?;

    let cid_hashed = get_digest(message_cbor.as_ref())?;

    let message_digest = Message::parse_slice(&cid_hashed)?;

    let (signature_rs, recovery_id) = sign(&message_digest, &secret_key);

    let mut signature = SignatureSECP256K1([0; SIGNATURE_RECOVERY_SIZE]);
    signature.0[..64].copy_from_slice(&signature_rs.serialize()[..]);
    signature.0[64] = recovery_id.serialize();

    Ok(signature)
}

fn transaction_sign_bls_raw(
    unsigned_message: &UnsignedMessage,
    private_key: &PrivateKey,
) -> Result<SignatureBLS, SignerError> {
    let sk = bls_signatures::PrivateKey::from_bytes(&private_key.0)?;

    //sign the message's signing bytes
    let signed = sk.sign(unsigned_message.to_signing_bytes());
    let v = signed.as_bytes();
    if v.len() != BLS_SIGNATURE_SIZE {
        return Err(SignerError::GenericString("Invalid Signature Length".to_string()));
    }
    let mut signature = SignatureBLS { 0: [0; BLS_SIGNATURE_SIZE] };
    signature.0.copy_from_slice(&v[..BLS_SIGNATURE_SIZE]);

    Ok(signature)
}

/// Sign a transaction and return a raw signature (RSV format).
///
/// # Arguments
///
/// * `unsigned_message_api` - an unsigned filecoin message
/// * `private_key` - a `PrivateKey`
///
pub fn transaction_sign_raw(
    unsigned_message_api: &UnsignedMessage,
    private_key: &PrivateKey,
) -> Result<Signature, SignerError> {
    // the `from` address protocol let us know which signing scheme to use
    match unsigned_message_api.from.protocol() {
        Protocol::Secp256k1 => {
            let signed = transaction_sign_secp56k1_raw(unsigned_message_api, private_key)?;
            Ok(Signature::SignatureSECP256K1(signed))
        }
        Protocol::BLS => {
            let signed = transaction_sign_bls_raw(unsigned_message_api, private_key)?;
            Ok(Signature::SignatureBLS(signed))
        }
        _ => Err(SignerError::GenericString("Unknown signing protocol".to_string())),
    }
}

pub fn transaction_sign(transaction: &str, private_key_base64: &str) -> String {
    let mut de = serde_json::Deserializer::from_str(transaction);
    let private_key = base64::decode(private_key_base64);
    if private_key.is_err() {
        return String::new();
    }
    let mut sk = PrivateKey { 0: [0; SECRET_KEY_SIZE] };
    sk.0.copy_from_slice(&private_key.unwrap()[..SECRET_KEY_SIZE]);
    let message_user_api = MessageAPI::deserialize(&mut de);
    if message_user_api.is_err() {
        return String::new();
    }

    let raw_signature = transaction_sign_raw(&message_user_api.unwrap(), &sk);
    if let Ok(raw_signature) = raw_signature {
        return base64::encode(raw_signature.as_bytes());
    }
    return String::new();
}
