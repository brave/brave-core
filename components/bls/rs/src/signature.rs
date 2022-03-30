use std::convert::TryFrom;

use blake2b_simd::Params;
use serde::{Deserialize, Serialize};
use bls_signatures::{Signature};
use libsecp256k1::{sign, Message};
use libsecp256k1::util::{
  SECRET_KEY_SIZE, SIGNATURE_SIZE
};
use thiserror::Error;
use forest_message::{UnsignedMessage};
use forest_encoding::{to_vec};
pub const SIGNATURE_RECOVERY_SIZE: usize = SIGNATURE_SIZE + 1;

pub const BLS_SIGNATURE_SIZE: usize = 96;

pub struct PrivateKey(pub [u8; SECRET_KEY_SIZE]);

use core::{array::TryFromSliceError, num::ParseIntError};

pub struct SignatureBLS(pub [u8; BLS_SIGNATURE_SIZE]);
pub struct SignatureSECP256K1(pub [u8; SIGNATURE_RECOVERY_SIZE]);

impl Signature {
  pub fn as_bytes(&self) -> Vec<u8> {
      match self {
          Signature::SignatureSECP256K1(sig_secp256k1) => sig_secp256k1.as_bytes(),
          Signature::SignatureBLS(sig_bls) => sig_bls.as_bytes(),
      }
  }
}

impl SignatureBLS {
  pub fn as_bytes(&self) -> Vec<u8> {
      self.0.to_vec()
  }
}

impl SignatureSECP256K1 {
  pub fn as_bytes(&self) -> Vec<u8> {
      self.0.to_vec()
  }
}

impl TryFrom<String> for PrivateKey {
    type Error = SignerError;

    fn try_from(s: String) -> Result<PrivateKey, Self::Error> {
        let v = base64::decode(&s)?;

        PrivateKey::try_from(v)
    }
}

impl TryFrom<Vec<u8>> for PrivateKey {
    type Error = SignerError;

    fn try_from(v: Vec<u8>) -> Result<PrivateKey, Self::Error> {
        if v.len() != SECRET_KEY_SIZE {
            return Err(SignerError::GenericString("Invalid Key Length".to_string()));
        }
        let mut sk = PrivateKey {
            0: [0; SECRET_KEY_SIZE],
        };
        sk.0.copy_from_slice(&v[..SECRET_KEY_SIZE]);
        Ok(sk)
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
    // Deserialize error
    #[error("Cannot deserilaize parameters | {0}")]
    DeserializeError(#[from] forest_encoding::Error),
    // forest encoding error
    #[error("Encoding error | {0}")]
    EncodingError(#[from] forest_encoding::error::Error),
}

/// Unsigned message api structure
#[cfg_attr(feature = "with-arbitrary", derive(arbitrary::Arbitrary))]
#[derive(Debug, Clone, Deserialize, PartialEq, Serialize)]
pub struct UnsignedMessageAPI {
    #[serde(alias = "To")]
    pub to: String,
    #[serde(alias = "From")]
    pub from: String,
    #[serde(alias = "Nonce")]
    pub nonce: u64,
    #[serde(alias = "Value")]
    pub value: String,

    #[serde(rename = "gaslimit")]
    #[serde(alias = "gasLimit")]
    #[serde(alias = "gas_limit")]
    #[serde(alias = "GasLimit")]
    pub gas_limit: i64,

    #[serde(rename = "gasfeecap")]
    #[serde(alias = "gasFeeCap")]
    #[serde(alias = "gas_fee_cap")]
    #[serde(alias = "GasFeeCap")]
    pub gas_fee_cap: String,

    #[serde(rename = "gaspremium")]
    #[serde(alias = "gasPremium")]
    #[serde(alias = "gas_premium")]
    #[serde(alias = "GasPremium")]
    pub gas_premium: String,

    #[serde(alias = "Method")]
    pub method: u64,
    #[serde(alias = "Params")]
    pub params: String,
}

static CID_PREFIX: &[u8] = &[0x01, 0x71, 0xa0, 0xe4, 0x02, 0x20];

/// transform a message into a hashed message ready to be signed and following Filecoin standard
pub fn get_digest(message: &[u8]) -> Result<[u8; 32], TryFromSliceError> {
    let message_hashed = Params::new()
        .hash_length(32)
        .to_state()
        .update(message)
        .finalize();

    let cid_hashed = Params::new()
        .hash_length(32)
        .to_state()
        .update(&CID_PREFIX)
        .update(message_hashed.as_bytes())
        .finalize();

    cid_hashed.as_bytes().try_into()
}
/// CBOR message in a buffer
pub struct CborBuffer(pub Vec<u8>);

impl AsRef<[u8]> for CborBuffer {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

/// Serialize a transaction and return a CBOR hexstring.
///
/// # Arguments
///
/// * `transaction` - a filecoin transaction
///
pub fn transaction_serialize(
  unsigned_message_arg: &UnsignedMessageAPI,
) -> Result<CborBuffer, SignerError> {
  let unsigned_message = UnsignedMessage::try_from(unsigned_message_arg)?;
  let message_cbor = CborBuffer(to_vec(&unsigned_message)?);
  Ok(message_cbor)
}

fn transaction_sign_secp56k1_raw(
  unsigned_message_api: &UnsignedMessageAPI,
  private_key: &PrivateKey,
) -> Result<SignatureSECP256K1, SignerError> {
  let message_cbor = transaction_serialize(unsigned_message_api)?;

  let secret_key = libsecp256k1::SecretKey::parse_slice(&private_key.0)?;

  let cid_hashed = get_digest(message_cbor.as_ref())?;

  let message_digest = Message::parse_slice(&cid_hashed)?;

  let (signature_rs, recovery_id) = sign(&message_digest, &secret_key);

  let mut signature = SignatureSECP256K1 { 0: [0; 65] };
  signature.0[..64].copy_from_slice(&signature_rs.serialize()[..]);
  signature.0[64] = recovery_id.serialize();

  Ok(signature)
}
fn transaction_sign_bls_raw(
  unsigned_message_api: &UnsignedMessageAPI,
  private_key: &PrivateKey,
) -> Result<SignatureBLS, SignerError> {
  let sk = bls_signatures::PrivateKey::from_bytes(&private_key.0)?;

  let unsigned_message = UnsignedMessage::try_from(unsigned_message_api)?;

  //sign the message's signing bytes
  let sig = sk.sign(unsigned_message.to_signing_bytes());

  Ok(SignatureBLS::try_from(sig.as_bytes())?)
}
/// Sign a transaction and return a raw signature (RSV format).
///
/// # Arguments
///
/// * `unsigned_message_api` - an unsigned filecoin message
/// * `private_key` - a `PrivateKey`
///
pub fn transaction_sign_raw(
  unsigned_message_api: &UnsignedMessageAPI,
  private_key: &PrivateKey,
) -> Result<Signature, SignerError> {
  // the `from` address protocol let us know which signing scheme to use
  let signature = match unsigned_message_api
      .from
      .as_bytes()
      .get(1)
      .ok_or_else(|| SignerError::GenericString("Empty signing protocol".into()))?
  {
      b'1' => Signature::SignatureSECP256K1(transaction_sign_secp56k1_raw(
          unsigned_message_api,
          private_key,
      )?),
      b'3' => {
          Signature::SignatureBLS(transaction_sign_bls_raw(unsigned_message_api, private_key)?)
      }
      _ => {
          return Err(SignerError::GenericString(
              "Unknown signing protocol".to_string(),
          ));
      }
  };

  Ok(signature)
}
