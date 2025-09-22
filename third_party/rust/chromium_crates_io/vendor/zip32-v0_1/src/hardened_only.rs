//! Generic framework for hardened-only key derivation.
//!
//! Defined in [ZIP32: Hardened-only key derivation][hkd].
//!
//! Any usage of the types in this module needs to have a corresponding ZIP. If you just
//! want to derive an arbitrary key in a ZIP 32-compatible manner without ecosystem-wide
//! coordination, use [`arbitrary::SecretKey`].
//!
//! [hkd]: https://zips.z.cash/zip-0032#specification-hardened-only-key-derivation
//! [`arbitrary::SecretKey`]: crate::arbitrary::SecretKey

use core::marker::PhantomData;

use blake2b_simd::Params as Blake2bParams;
use subtle::{Choice, ConstantTimeEq};
use zcash_spec::PrfExpand;

use crate::{ChainCode, ChildIndex};

/// The context in which hardened-only key derivation is instantiated.
pub trait Context {
    /// A 16-byte domain separator used during master key generation.
    ///
    /// It SHOULD be disjoint from other domain separators used with BLAKE2b in Zcash
    /// protocols.
    const MKG_DOMAIN: [u8; 16];
    /// The `PrfExpand` domain used during child key derivation.
    const CKD_DOMAIN: PrfExpand<([u8; 32], [u8; 4])>;
}

/// An arbitrary extended secret key.
///
/// Defined in [ZIP32: Hardened-only key derivation][hkd].
///
/// [hkd]: https://zips.z.cash/zip-0032#specification-hardened-only-key-derivation
#[derive(Clone, Debug)]
pub struct HardenedOnlyKey<C: Context> {
    sk: [u8; 32],
    chain_code: ChainCode,
    _context: PhantomData<C>,
}

impl<C: Context> ConstantTimeEq for HardenedOnlyKey<C> {
    fn ct_eq(&self, rhs: &Self) -> Choice {
        self.chain_code.ct_eq(&rhs.chain_code) & self.sk.ct_eq(&rhs.sk)
    }
}

#[allow(non_snake_case)]
impl<C: Context> HardenedOnlyKey<C> {
    /// Exposes the parts of this key.
    pub fn parts(&self) -> (&[u8; 32], &ChainCode) {
        (&self.sk, &self.chain_code)
    }

    /// Decomposes this key into its parts.
    pub(crate) fn into_parts(self) -> ([u8; 32], ChainCode) {
        (self.sk, self.chain_code)
    }

    /// Generates the master key of a hardened-only extended secret key.
    ///
    /// Defined in [ZIP32: Hardened-only master key generation][mkgh].
    ///
    /// [mkgh]: https://zips.z.cash/zip-0032#hardened-only-master-key-generation
    pub fn master(ikm: &[&[u8]]) -> Self {
        // I := BLAKE2b-512(Context.MKGDomain, IKM)
        let I: [u8; 64] = {
            let mut I = Blake2bParams::new()
                .hash_length(64)
                .personal(&C::MKG_DOMAIN)
                .to_state();
            for input in ikm {
                I.update(input);
            }
            I.finalize().as_bytes().try_into().unwrap()
        };

        let (I_L, I_R) = I.split_at(32);

        // I_L is used as the master secret key sk_m.
        let sk_m = I_L.try_into().unwrap();

        // I_R is used as the master chain code c_m.
        let c_m = ChainCode::new(I_R.try_into().unwrap());

        Self {
            sk: sk_m,
            chain_code: c_m,
            _context: PhantomData,
        }
    }

    /// Derives a child key from a parent key at a given index.
    ///
    /// Defined in [ZIP32: Hardened-only child key derivation][ckdh].
    ///
    /// [ckdh]: https://zips.z.cash/zip-0032#hardened-only-child-key-derivation
    pub fn derive_child(&self, index: ChildIndex) -> Self {
        // I := PRF^Expand(c_par, [Context.CKDDomain] || sk_par || I2LEOSP(i))
        let I: [u8; 64] = C::CKD_DOMAIN.with(
            self.chain_code.as_bytes(),
            &self.sk,
            &index.index().to_le_bytes(),
        );

        let (I_L, I_R) = I.split_at(32);

        // I_L is used as the child spending key sk_i.
        let sk_i = I_L.try_into().unwrap();

        // I_R is used as the child chain code c_i.
        let c_i = ChainCode::new(I_R.try_into().unwrap());

        Self {
            sk: sk_i,
            chain_code: c_i,
            _context: PhantomData,
        }
    }
}
