use core::fmt;
use ed25519_dalek_bip32::derivation_path::ChildIndexError;
use ed25519_dalek_bip32::derivation_path::DerivationPath;
use ed25519_dalek_bip32::derivation_path::DerivationPathParseError;
use ed25519_dalek_bip32::ed25519_dalek::KEYPAIR_LENGTH;
use ed25519_dalek_bip32::ed25519_dalek::PUBLIC_KEY_LENGTH;
use ed25519_dalek_bip32::ed25519_dalek::SECRET_KEY_LENGTH;
use ed25519_dalek_bip32::ChildIndex;
use ed25519_dalek_bip32::ExtendedSecretKey;

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        type Ed25519DalekExtendedSecretKey;
        fn generate_ed25519_extended_secrect_key_from_seed(
            bytes: &[u8],
        ) -> Box<Ed25519DalekExtendedSecretKey>;
        fn derive(&self, path: String) -> Box<Ed25519DalekExtendedSecretKey>;
        fn derive_child(&self, index: u32) -> Box<Ed25519DalekExtendedSecretKey>;
        fn keypair_raw(&self) -> [u8; 64];
        fn public_key_raw(&self) -> [u8; 32];
        fn is_valid(&self) -> bool;
        fn error_message(&self) -> String;
    }
}

#[derive(Debug)]
pub enum Error {
    InvalidKey,
    Ed25519Bip32(ed25519_dalek_bip32::Error),
    DerivationPathParse(DerivationPathParseError),
    ChildIndex(ChildIndexError),
}

impl From<ed25519_dalek_bip32::Error> for Error {
    fn from(err: ed25519_dalek_bip32::Error) -> Self {
        Self::Ed25519Bip32(err)
    }
}

impl From<DerivationPathParseError> for Error {
    fn from(err: DerivationPathParseError) -> Self {
        Self::DerivationPathParse(err)
    }
}

impl From<ChildIndexError> for Error {
    fn from(err: ChildIndexError) -> Self {
        Self::ChildIndex(err)
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self {
            Error::InvalidKey => write!(f, "Ed25519DalekExtendedSecretKey is invalid"),
            Error::Ed25519Bip32(e) => write!(f, "Error: {}", e.to_string()),
            Error::DerivationPathParse(e) => write!(f, "Error: {}", e.to_string()),
            Error::ChildIndex(e) => write!(f, "Error: {}", e.to_string()),
        }
    }
}

pub struct Ed25519DalekExtendedSecretKey {
    key: Option<ExtendedSecretKey>,
    error: Option<Error>,
}

fn generate_ed25519_extended_secrect_key_from_seed(
    bytes: &[u8],
) -> Box<Ed25519DalekExtendedSecretKey> {
    Box::new(Ed25519DalekExtendedSecretKey::from(
        ExtendedSecretKey::from_seed(bytes).map_err(|err| Error::from(err)),
    ))
}

impl Ed25519DalekExtendedSecretKey {
    fn derive(&self, path: String) -> Box<Ed25519DalekExtendedSecretKey> {
        Box::new(Ed25519DalekExtendedSecretKey::from(
            self.key.as_ref().ok_or_else(|| Error::InvalidKey).and_then(|key| {
                let d_path = path.parse::<DerivationPath>()?;
                Ok(key.derive(&d_path)?)
            }),
        ))
    }
    fn derive_child(&self, index: u32) -> Box<Ed25519DalekExtendedSecretKey> {
        Box::new(Ed25519DalekExtendedSecretKey::from(
            self.key.as_ref().ok_or_else(|| Error::InvalidKey).and_then(|key| {
                let child_index = ChildIndex::hardened(index)?;
                Ok(key.derive_child(child_index)?)
            }),
        ))
    }
    fn keypair_raw(&self) -> [u8; KEYPAIR_LENGTH] {
        self.key.as_ref().map_or_else(
            || [0u8; KEYPAIR_LENGTH],
            |key| {
                let mut bytes: [u8; KEYPAIR_LENGTH] = [0u8; KEYPAIR_LENGTH];
                bytes[..SECRET_KEY_LENGTH].copy_from_slice(&key.secret_key.to_bytes());
                bytes[SECRET_KEY_LENGTH..].copy_from_slice(&key.public_key().to_bytes());
                bytes
            },
        )
    }
    fn public_key_raw(&self) -> [u8; PUBLIC_KEY_LENGTH] {
        self.key
            .as_ref()
            .map_or_else(|| [0u8; PUBLIC_KEY_LENGTH], |key| key.public_key().to_bytes())
    }
    fn is_valid(&self) -> bool {
        match self.key {
            Some(_) => true,
            None => false,
        }
    }
    fn error_message(&self) -> String {
        match &self.error {
            Some(v) => v.to_string(),
            None => "".to_string(),
        }
    }
}

impl From<Result<ExtendedSecretKey, Error>> for Ed25519DalekExtendedSecretKey {
    fn from(result: Result<ExtendedSecretKey, Error>) -> Self {
        match result {
            Ok(v) => Self { key: Some(v), error: None },
            Err(e) => Self { key: None, error: Some(e) },
        }
    }
}
