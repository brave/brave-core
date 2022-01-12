use ed25519_dalek_bip32::derivation_path::DerivationPath;
use ed25519_dalek_bip32::ed25519_dalek::Keypair;
use ed25519_dalek_bip32::ed25519_dalek::SecretKey;
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
    }
}

pub struct Ed25519DalekExtendedSecretKey(Option<ExtendedSecretKey>);

fn generate_ed25519_extended_secrect_key_from_seed(
    bytes: &[u8],
) -> Box<Ed25519DalekExtendedSecretKey> {
    Box::new(Ed25519DalekExtendedSecretKey(Some(ExtendedSecretKey::from_seed(bytes).unwrap())))
}

impl Ed25519DalekExtendedSecretKey {
    fn derive(&self, path: String) -> Box<Ed25519DalekExtendedSecretKey> {
        let d_path: Result<DerivationPath, _> = path.parse();
        if d_path.is_err() {
            return Box::new(Ed25519DalekExtendedSecretKey(None));
        }
        let child = self.0.as_ref().unwrap().derive(&d_path.unwrap());
        if child.is_ok() {
            Box::new(Ed25519DalekExtendedSecretKey(Some(child.unwrap())))
        } else {
            Box::new(Ed25519DalekExtendedSecretKey(None))
        }
    }
    fn derive_child(&self, index: u32) -> Box<Ed25519DalekExtendedSecretKey> {
        let child_index: Result<ChildIndex, _> = ChildIndex::hardened(index);
        if child_index.is_err() {
            Box::new(Ed25519DalekExtendedSecretKey(None))
        } else {
            Box::new(Ed25519DalekExtendedSecretKey(Some(
                self.0.as_ref().unwrap().derive_child(child_index.unwrap()).unwrap(),
            )))
        }
    }
    fn keypair_raw(&self) -> [u8; 64] {
        let secret_key =
            SecretKey::from_bytes(&self.0.as_ref().unwrap().secret_key.to_bytes()).unwrap();
        Keypair { secret: secret_key, public: self.0.as_ref().unwrap().public_key() }.to_bytes()
    }
    fn public_key_raw(&self) -> [u8; 32] {
        self.0.as_ref().unwrap().public_key().to_bytes()
    }

    fn is_valid(&self) -> bool {
        match &self.0 {
            None => false,
            Some(_) => true,
        }
    }
}
