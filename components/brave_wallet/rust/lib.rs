use core::fmt;
use ed25519_dalek_bip32::derivation_path::ChildIndexError;
use ed25519_dalek_bip32::derivation_path::DerivationPath;
use ed25519_dalek_bip32::derivation_path::DerivationPathParseError;
use ed25519_dalek_bip32::ed25519_dalek::KEYPAIR_LENGTH;
use ed25519_dalek_bip32::ed25519_dalek::PUBLIC_KEY_LENGTH;
use ed25519_dalek_bip32::ed25519_dalek::SECRET_KEY_LENGTH;
use ed25519_dalek_bip32::ChildIndex;
use ed25519_dalek_bip32::ExtendedSecretKey;

macro_rules! impl_result {
    ($t:ident, $r:ident) => {
        impl $r {
            fn error_message(self: &$r) -> String {
                match &self.0 {
                    Err(e) => e.to_string(),
                    Ok(_) => "".to_string(),
                }
            }

            fn is_ok(self: &$r) -> bool {
                match &self.0 {
                    Err(_) => false,
                    Ok(_) => true,
                }
            }

            fn unwrap(self: &$r) -> &$t {
                self.0.as_ref().expect("Unhandled error before unwrap call")
            }
        }
    };
}

#[cxx::bridge(namespace =  brave_wallet)]
mod ffi {
    extern "Rust" {
        type Ed25519DalekExtendedSecretKey;
        type Ed25519DalekExtendedSecretKeyResult;

        fn generate_ed25519_extended_secrect_key_from_seed(
            bytes: &[u8],
        ) -> Box<Ed25519DalekExtendedSecretKeyResult>;
        fn derive(
            self: &Ed25519DalekExtendedSecretKey,
            path: String,
        ) -> Box<Ed25519DalekExtendedSecretKeyResult>;
        fn derive_child(
            self: &Ed25519DalekExtendedSecretKey,
            index: u32,
        ) -> Box<Ed25519DalekExtendedSecretKeyResult>;
        fn keypair_raw(self: &Ed25519DalekExtendedSecretKey) -> [u8; 64];
        fn public_key_raw(self: &Ed25519DalekExtendedSecretKey) -> [u8; 32];

        fn is_ok(self: &Ed25519DalekExtendedSecretKeyResult) -> bool;
        fn error_message(self: &Ed25519DalekExtendedSecretKeyResult) -> String;
        fn unwrap(self: &Ed25519DalekExtendedSecretKeyResult) -> &Ed25519DalekExtendedSecretKey;
    }
}

#[derive(Debug)]
pub enum Error {
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
            Error::Ed25519Bip32(e) => write!(f, "Error: {}", e.to_string()),
            Error::DerivationPathParse(e) => write!(f, "Error: {}", e.to_string()),
            Error::ChildIndex(e) => write!(f, "Error: {}", e.to_string()),
        }
    }
}

pub struct Ed25519DalekExtendedSecretKey(ExtendedSecretKey);

struct Ed25519DalekExtendedSecretKeyResult(Result<Ed25519DalekExtendedSecretKey, Error>);

impl_result!(Ed25519DalekExtendedSecretKey, Ed25519DalekExtendedSecretKeyResult);

impl From<Result<ExtendedSecretKey, Error>> for Ed25519DalekExtendedSecretKeyResult {
    fn from(key: Result<ExtendedSecretKey, Error>) -> Self {
        match key {
            Ok(v) => Self(Ok(Ed25519DalekExtendedSecretKey(v))),
            Err(e) => Self(Err(e)),
        }
    }
}

fn generate_ed25519_extended_secrect_key_from_seed(
    bytes: &[u8],
) -> Box<Ed25519DalekExtendedSecretKeyResult> {
    Box::new(Ed25519DalekExtendedSecretKeyResult::from(
        ExtendedSecretKey::from_seed(bytes).map_err(|err| Error::from(err)),
    ))
}

impl Ed25519DalekExtendedSecretKey {
    fn derive(&self, path: String) -> Box<Ed25519DalekExtendedSecretKeyResult> {
        Box::new(Ed25519DalekExtendedSecretKeyResult::from(
            path.parse::<DerivationPath>()
                .map_err(|err| Error::from(err))
                .and_then(|d_path| Ok(self.0.derive(&d_path)?)),
        ))
    }
    fn derive_child(&self, index: u32) -> Box<Ed25519DalekExtendedSecretKeyResult> {
        Box::new(Ed25519DalekExtendedSecretKeyResult::from(
            ChildIndex::hardened(index)
                .map_err(|err| Error::from(err))
                .and_then(|child_index| Ok(self.0.derive_child(child_index)?)),
        ))
    }
    fn keypair_raw(&self) -> [u8; KEYPAIR_LENGTH] {
        let mut bytes: [u8; KEYPAIR_LENGTH] = [0u8; KEYPAIR_LENGTH];
        bytes[..SECRET_KEY_LENGTH].copy_from_slice(&self.0.secret_key.to_bytes());
        bytes[SECRET_KEY_LENGTH..].copy_from_slice(&self.0.public_key().to_bytes());
        bytes
    }
    fn public_key_raw(&self) -> [u8; PUBLIC_KEY_LENGTH] {
        self.0.public_key().to_bytes()
    }
}
