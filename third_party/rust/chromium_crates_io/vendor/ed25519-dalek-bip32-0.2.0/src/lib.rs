//! A simple BIP32 implementation for ed25519 public keys. Although there exists [another very good
//! library that does this](https://docs.rs/ed25519-bip32), this library preserves 32 byte secret
//! keys and doesn't allow for extended public keys or "normal" child indexes, so that it can be as
//! close to the BIP32 specifications as possible, allowing for compatibility with libraries like
//! `trezor-crypto`

#![cfg_attr(not(feature = "std"), no_std)]

pub extern crate derivation_path;
pub extern crate ed25519_dalek;

pub use derivation_path::{ChildIndex, DerivationPath};
pub use ed25519_dalek::{PublicKey, SecretKey};

use core::fmt;
use hmac::{Hmac, Mac};
use sha2::Sha512;

const ED25519_BIP32_NAME: &str = "ed25519 seed";

/// Errors thrown while deriving secret keys
#[derive(Debug)]
pub enum Error {
    Ed25519,
    ExpectedHardenedIndex(ChildIndex),
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Ed25519 => f.write_str("ed25519 error"),
            Self::ExpectedHardenedIndex(index) => {
                f.write_fmt(format_args!("expected hardened child index: {}", index))
            }
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}

/// An expanded secret key with chain code and meta data
#[derive(Debug)]
pub struct ExtendedSecretKey {
    /// How many derivations this key is from the root (0 for root)
    pub depth: u8,
    /// Child index of the key used to derive from parent (`Normal(0)` for root)
    pub child_index: ChildIndex,
    /// Secret Key
    pub secret_key: SecretKey,
    /// Chain code
    pub chain_code: [u8; 32],
}

type HmacSha512 = Hmac<Sha512>;

/// A convenience wrapper for a [`core::result::Result`] with an [`Error`]
pub type Result<T, E = Error> = core::result::Result<T, E>;

impl ExtendedSecretKey {
    /// Create a new extended secret key from a seed
    pub fn from_seed(seed: &[u8]) -> Result<Self> {
        let mut mac = HmacSha512::new_from_slice(ED25519_BIP32_NAME.as_ref()).unwrap();
        mac.update(seed);
        let bytes = mac.finalize().into_bytes();

        let secret_key = SecretKey::from_bytes(&bytes[..32])?;
        let mut chain_code = [0; 32];
        chain_code.copy_from_slice(&bytes[32..]);

        Ok(Self {
            depth: 0,
            child_index: ChildIndex::Normal(0),
            secret_key,
            chain_code,
        })
    }

    /// Derive an extended secret key fom the current using a derivation path
    pub fn derive<P: AsRef<[ChildIndex]>>(&self, path: &P) -> Result<Self> {
        let mut path = path.as_ref().iter();
        let mut next = match path.next() {
            Some(index) => self.derive_child(*index)?,
            None => self.clone(),
        };
        for index in path {
            next = next.derive_child(*index)?;
        }
        Ok(next)
    }

    /// Derive a child extended secret key with an index
    pub fn derive_child(&self, index: ChildIndex) -> Result<Self> {
        if index.is_normal() {
            return Err(Error::ExpectedHardenedIndex(index));
        }

        let mut mac = HmacSha512::new_from_slice(&self.chain_code).unwrap();
        mac.update(&[0u8]);
        mac.update(self.secret_key.to_bytes().as_ref());
        mac.update(index.to_bits().to_be_bytes().as_ref());
        let bytes = mac.finalize().into_bytes();

        let secret_key = SecretKey::from_bytes(&bytes[..32])?;
        let mut chain_code = [0; 32];
        chain_code.copy_from_slice(&bytes[32..]);

        Ok(Self {
            depth: self.depth + 1,
            child_index: index,
            secret_key,
            chain_code,
        })
    }

    /// Get the associated public key
    #[inline]
    pub fn public_key(&self) -> PublicKey {
        PublicKey::from(&self.secret_key)
    }

    #[inline]
    fn clone(&self) -> Self {
        Self {
            depth: self.depth,
            child_index: self.child_index,
            secret_key: SecretKey::from_bytes(&self.secret_key.to_bytes()).unwrap(),
            chain_code: self.chain_code,
        }
    }
}

impl From<ed25519_dalek::SignatureError> for Error {
    fn from(_: ed25519_dalek::SignatureError) -> Self {
        Self::Ed25519
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    extern crate alloc;
    use alloc::vec::Vec;

    fn hex_str(string: &str) -> Vec<u8> {
        hex::decode(string).unwrap()
    }

    fn root(seed: &str) -> ExtendedSecretKey {
        ExtendedSecretKey::from_seed(&hex_str(seed)).unwrap()
    }

    #[test]
    fn derivation_path() {
        let vector1_path: DerivationPath = "m/0'/1'/2'/2'/1000000000'".parse().unwrap();
        let vector2_path: DerivationPath = "m/0'/2147483647'/1'/2147483646'/2'".parse().unwrap();

        let node = root("000102030405060708090a0b0c0d0e0f")
            .derive(&vector1_path)
            .unwrap();
        assert_eq!(node.depth, 5);
        assert_eq!(node.child_index, ChildIndex::Hardened(1000000000));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("68789923a0cac2cd5a29172a475fe9e0fb14cd6adb5ad98a3fa70333e7afa230")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "8f94d394a8e8fd6b1bc2f3f49f5c47e385281d5c17e65324b0f62483e37e8793",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "3c24da049451555d51a7014a37337aa4e12d41e485abccfa46b47dfb2af54b7a",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        let node = root("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542").derive(&vector2_path).unwrap();
        assert_eq!(node.depth, 5);
        assert_eq!(node.child_index, ChildIndex::Hardened(2));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("5d70af781f3a37b829f0d060924d5e960bdc02e85423494afc0b1a41bbe196d4")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "551d333177df541ad876a60ea71f00447931c0a9da16f227c11ea080d7391b8d",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "47150c75db263559a70d5778bf36abbab30fb061ad69f69ece61a72b0cfa4fc0",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());
    }

    #[test]
    fn normal_errs() {
        let node = root("000102030405060708090a0b0c0d0e0f");

        let res = node.derive_child(ChildIndex::Normal(0));
        assert!(matches!(
            res,
            Err(Error::ExpectedHardenedIndex(ChildIndex::Normal(0)))
        ));

        let res = node.derive_child(ChildIndex::Normal(100000));
        assert!(matches!(
            res,
            Err(Error::ExpectedHardenedIndex(ChildIndex::Normal(100000)))
        ));

        let soft_path: DerivationPath = "m/0'/1'/2'/3/4'".parse().unwrap();
        let res = node.derive(&soft_path);
        assert!(matches!(
            res,
            Err(Error::ExpectedHardenedIndex(ChildIndex::Normal(3)))
        ));
    }

    #[test]
    fn vector1() {
        // Chain m
        let node = root("000102030405060708090a0b0c0d0e0f");
        assert_eq!(node.depth, 0);
        assert_eq!(node.child_index, ChildIndex::Normal(0));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("90046a93de5380a72b5e45010748567d5ea02bbf6522f979e05c0d8d8ca9fffb")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "2b4be7f19ee27bbf30c667b642d5f4aa69fd169872f8fc3059c08ebae2eb19e7",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "a4b2856bfec510abab89753fac1ac0e1112364e7d250545963f135f2a33188ed",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'
        let node = node.derive_child(ChildIndex::Hardened(0)).unwrap();
        assert_eq!(node.depth, 1);
        assert_eq!(node.child_index, ChildIndex::Hardened(0));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("8b59aa11380b624e81507a27fedda59fea6d0b779a778918a2fd3590e16e9c69")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "68e0fe46dfb67e368c75379acec591dad19df3cde26e63b93a8e704f1dade7a3",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "8c8a13df77a28f3445213a0f432fde644acaa215fc72dcdf300d5efaa85d350c",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'/1'
        let node = node.derive_child(ChildIndex::Hardened(1)).unwrap();
        assert_eq!(node.depth, 2);
        assert_eq!(node.child_index, ChildIndex::Hardened(1));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("a320425f77d1b5c2505a6b1b27382b37368ee640e3557c315416801243552f14")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "b1d0bad404bf35da785a64ca1ac54b2617211d2777696fbffaf208f746ae84f2",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "1932a5270f335bed617d5b935c80aedb1a35bd9fc1e31acafd5372c30f5c1187",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'/1'/2'
        let node = node.derive_child(ChildIndex::Hardened(2)).unwrap();
        assert_eq!(node.depth, 3);
        assert_eq!(node.child_index, ChildIndex::Hardened(2));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("2e69929e00b5ab250f49c3fb1c12f252de4fed2c1db88387094a0f8c4c9ccd6c")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "92a5b23c0b8a99e37d07df3fb9966917f5d06e02ddbd909c7e184371463e9fc9",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "ae98736566d30ed0e9d2f4486a64bc95740d89c7db33f52121f8ea8f76ff0fc1",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'/1'/2'/2'
        let node = node.derive_child(ChildIndex::Hardened(2)).unwrap();
        assert_eq!(node.depth, 4);
        assert_eq!(node.child_index, ChildIndex::Hardened(2));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("8f6d87f93d750e0efccda017d662a1b31a266e4a6f5993b15f5c1f07f74dd5cc")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "30d1dc7e5fc04c31219ab25a27ae00b50f6fd66622f6e9c913253d6511d1e662",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "8abae2d66361c879b900d204ad2cc4984fa2aa344dd7ddc46007329ac76c429c",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'/1'/2'/2'/1000000000'
        let node = node.derive_child(ChildIndex::Hardened(1000000000)).unwrap();
        assert_eq!(node.depth, 5);
        assert_eq!(node.child_index, ChildIndex::Hardened(1000000000));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("68789923a0cac2cd5a29172a475fe9e0fb14cd6adb5ad98a3fa70333e7afa230")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "8f94d394a8e8fd6b1bc2f3f49f5c47e385281d5c17e65324b0f62483e37e8793",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "3c24da049451555d51a7014a37337aa4e12d41e485abccfa46b47dfb2af54b7a",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());
    }

    #[test]
    fn vector2() {
        // Chain m
        let node = root("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542");
        assert_eq!(node.depth, 0);
        assert_eq!(node.child_index, ChildIndex::Normal(0));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("ef70a74db9c3a5af931b5fe73ed8e1a53464133654fd55e7a66f8570b8e33c3b")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "171cb88b1b3c1db25add599712e36245d75bc65a1a5c9e18d76f9f2b1eab4012",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "8fe9693f8fa62a4305a140b9764c5ee01e455963744fe18204b4fb948249308a",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'
        let node = node.derive_child(ChildIndex::Hardened(0)).unwrap();
        assert_eq!(node.depth, 1);
        assert_eq!(node.child_index, ChildIndex::Hardened(0));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("0b78a3226f915c082bf118f83618a618ab6dec793752624cbeb622acb562862d")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "1559eb2bbec5790b0c65d8693e4d0875b1747f4970ae8b650486ed7470845635",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "86fab68dcb57aa196c77c5f264f215a112c22a912c10d123b0d03c3c28ef1037",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'/2147483647'
        let node = node.derive_child(ChildIndex::Hardened(2147483647)).unwrap();
        assert_eq!(node.depth, 2);
        assert_eq!(node.child_index, ChildIndex::Hardened(2147483647));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("138f0b2551bcafeca6ff2aa88ba8ed0ed8de070841f0c4ef0165df8181eaad7f")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "ea4f5bfe8694d8bb74b7b59404632fd5968b774ed545e810de9c32a4fb4192f4",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "5ba3b9ac6e90e83effcd25ac4e58a1365a9e35a3d3ae5eb07b9e4d90bcf7506d",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'/2147483647'/1'
        let node = node.derive_child(ChildIndex::Hardened(1)).unwrap();
        assert_eq!(node.depth, 3);
        assert_eq!(node.child_index, ChildIndex::Hardened(1));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("73bd9fff1cfbde33a1b846c27085f711c0fe2d66fd32e139d3ebc28e5a4a6b90")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "3757c7577170179c7868353ada796c839135b3d30554bbb74a4b1e4a5a58505c",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "2e66aa57069c86cc18249aecf5cb5a9cebbfd6fadeab056254763874a9352b45",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'/2147483647'/1'/2147483646'
        let node = node.derive_child(ChildIndex::Hardened(2147483646)).unwrap();
        assert_eq!(node.depth, 4);
        assert_eq!(node.child_index, ChildIndex::Hardened(2147483646));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("0902fe8a29f9140480a00ef244bd183e8a13288e4412d8389d140aac1794825a")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "5837736c89570de861ebc173b1086da4f505d4adb387c6a1b1342d5e4ac9ec72",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "e33c0f7d81d843c572275f287498e8d408654fdf0d1e065b84e2e6f157aab09b",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());

        // Chain m/0'/2147483647'/1'/2147483646'/2'
        let node = node.derive_child(ChildIndex::Hardened(2)).unwrap();
        assert_eq!(node.depth, 5);
        assert_eq!(node.child_index, ChildIndex::Hardened(2));
        assert_eq!(
            node.chain_code.as_ref(),
            hex_str("5d70af781f3a37b829f0d060924d5e960bdc02e85423494afc0b1a41bbe196d4")
        );
        let secret = SecretKey::from_bytes(&hex_str(
            "551d333177df541ad876a60ea71f00447931c0a9da16f227c11ea080d7391b8d",
        ))
        .unwrap();
        assert_eq!(node.secret_key.to_bytes(), secret.to_bytes());
        let public = PublicKey::from_bytes(&hex_str(
            "47150c75db263559a70d5778bf36abbab30fb061ad69f69ece61a72b0cfa4fc0",
        ))
        .unwrap();
        assert_eq!(node.public_key().to_bytes(), public.to_bytes());
    }
}
