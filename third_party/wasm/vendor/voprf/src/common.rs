// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Common functionality between multiple OPRF modes.

use core::convert::TryFrom;
use core::ops::Add;

use derive_where::derive_where;
use digest::core_api::BlockSizeUser;
use digest::{Digest, Output, OutputSizeUser};
use generic_array::sequence::Concat;
use generic_array::typenum::{IsLess, IsLessOrEqual, Unsigned, U2, U256, U9};
use generic_array::{ArrayLength, GenericArray};
use rand_core::{CryptoRng, RngCore};
use subtle::ConstantTimeEq;

#[cfg(feature = "serde")]
use crate::serialization::serde::{Element, Scalar};
use crate::{CipherSuite, Error, Group, InternalError, Result};

///////////////
// Constants //
// ========= //
///////////////

pub(crate) const STR_FINALIZE: [u8; 8] = *b"Finalize";
pub(crate) const STR_SEED: [u8; 5] = *b"Seed-";
pub(crate) const STR_DERIVE_KEYPAIR: [u8; 13] = *b"DeriveKeyPair";
pub(crate) const STR_COMPOSITE: [u8; 9] = *b"Composite";
pub(crate) const STR_CHALLENGE: [u8; 9] = *b"Challenge";
pub(crate) const STR_INFO: [u8; 4] = *b"Info";
pub(crate) const STR_OPRF: [u8; 7] = *b"OPRFV1-";
pub(crate) const STR_HASH_TO_SCALAR: [u8; 13] = *b"HashToScalar-";
pub(crate) const STR_HASH_TO_GROUP: [u8; 12] = *b"HashToGroup-";

/// Determines the mode of operation (either base mode or verifiable mode). This
/// is only used for custom implementations for [`Group`].
#[derive(Clone, Copy, Debug)]
pub enum Mode {
    /// Non-verifiable mode.
    Oprf,
    /// Verifiable mode.
    Voprf,
    /// Partially-oblivious mode.
    Poprf,
}

impl Mode {
    /// Mode as it is represented in a context string.
    pub fn to_u8(self) -> u8 {
        match self {
            Mode::Oprf => 0,
            Mode::Voprf => 1,
            Mode::Poprf => 2,
        }
    }
}

////////////////////////////
// High-level API Structs //
// ====================== //
////////////////////////////

/// The first client message sent from a client (either verifiable or not) to a
/// server (either verifiable or not).
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Elem)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct BlindedElement<CS: CipherSuite>(
    #[cfg_attr(feature = "serde", serde(with = "Element::<CS::Group>"))]
    pub(crate)  <CS::Group as Group>::Elem,
)
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>;

/// The server's response to the [BlindedElement] message from a client (either
/// verifiable or not) to a server (either verifiable or not).
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Elem)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct EvaluationElement<CS: CipherSuite>(
    #[cfg_attr(feature = "serde", serde(with = "Element::<CS::Group>"))]
    pub(crate)  <CS::Group as Group>::Elem,
)
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>;

/// Contains prepared [`EvaluationElement`]s by a server batch evaluate
/// preparation.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Elem)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct PreparedEvaluationElement<CS: CipherSuite>(pub(crate) EvaluationElement<CS>)
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>;

/// A proof produced by a server that the OPRF output matches against a server
/// public key.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Scalar)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct Proof<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    #[cfg_attr(feature = "serde", serde(with = "Scalar::<CS::Group>"))]
    pub(crate) c_scalar: <CS::Group as Group>::Scalar,
    #[cfg_attr(feature = "serde", serde(with = "Scalar::<CS::Group>"))]
    pub(crate) s_scalar: <CS::Group as Group>::Scalar,
}

/////////////////////
// Proof Functions //
// =============== //
/////////////////////

/// Can only fail with [`Error::Batch`].
#[allow(clippy::many_single_char_names)]
pub(crate) fn generate_proof<CS: CipherSuite, R: RngCore + CryptoRng>(
    rng: &mut R,
    k: <CS::Group as Group>::Scalar,
    a: <CS::Group as Group>::Elem,
    b: <CS::Group as Group>::Elem,
    cs: impl Iterator<Item = <CS::Group as Group>::Elem> + ExactSizeIterator,
    ds: impl Iterator<Item = <CS::Group as Group>::Elem> + ExactSizeIterator,
    mode: Mode,
) -> Result<Proof<CS>>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    // https://www.rfc-editor.org/rfc/rfc9497#section-2.2.1

    let (m, z) = compute_composites::<CS, _, _>(Some(k), b, cs, ds, mode)?;

    let r = CS::Group::random_scalar(rng);
    let t2 = a * &r;
    let t3 = m * &r;

    // Bm = GG.SerializeElement(B)
    let bm = CS::Group::serialize_elem(b);
    // a0 = GG.SerializeElement(M)
    let a0 = CS::Group::serialize_elem(m);
    // a1 = GG.SerializeElement(Z)
    let a1 = CS::Group::serialize_elem(z);
    // a2 = GG.SerializeElement(t2)
    let a2 = CS::Group::serialize_elem(t2);
    // a3 = GG.SerializeElement(t3)
    let a3 = CS::Group::serialize_elem(t3);

    let elem_len = <CS::Group as Group>::ElemLen::U16.to_be_bytes();

    // h2Input = I2OSP(len(Bm), 2) || Bm ||
    //           I2OSP(len(a0), 2) || a0 ||
    //           I2OSP(len(a1), 2) || a1 ||
    //           I2OSP(len(a2), 2) || a2 ||
    //           I2OSP(len(a3), 2) || a3 ||
    //           "Challenge"
    let h2_input = [
        &elem_len,
        bm.as_slice(),
        &elem_len,
        &a0,
        &elem_len,
        &a1,
        &elem_len,
        &a2,
        &elem_len,
        &a3,
        &STR_CHALLENGE,
    ];

    let dst = Dst::new::<CS, _, _>(STR_HASH_TO_SCALAR, mode);
    // This can't fail, the size of the `input` is known.
    let c_scalar = CS::Group::hash_to_scalar::<CS::Hash>(&h2_input, &dst.as_dst()).unwrap();
    let s_scalar = r - &(c_scalar * &k);

    Ok(Proof { c_scalar, s_scalar })
}

/// Can only fail with [`Error::ProofVerification`] or [`Error::Batch`].
#[allow(clippy::many_single_char_names)]
pub(crate) fn verify_proof<CS: CipherSuite>(
    a: <CS::Group as Group>::Elem,
    b: <CS::Group as Group>::Elem,
    cs: impl Iterator<Item = <CS::Group as Group>::Elem> + ExactSizeIterator,
    ds: impl Iterator<Item = <CS::Group as Group>::Elem> + ExactSizeIterator,
    proof: &Proof<CS>,
    mode: Mode,
) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    // https://www.rfc-editor.org/rfc/rfc9497#section-2.2.2
    let (m, z) = compute_composites::<CS, _, _>(None, b, cs, ds, mode)?;
    let t2 = (a * &proof.s_scalar) + &(b * &proof.c_scalar);
    let t3 = (m * &proof.s_scalar) + &(z * &proof.c_scalar);

    // Bm = GG.SerializeElement(B)
    let bm = CS::Group::serialize_elem(b);
    // a0 = GG.SerializeElement(M)
    let a0 = CS::Group::serialize_elem(m);
    // a1 = GG.SerializeElement(Z)
    let a1 = CS::Group::serialize_elem(z);
    // a2 = GG.SerializeElement(t2)
    let a2 = CS::Group::serialize_elem(t2);
    // a3 = GG.SerializeElement(t3)
    let a3 = CS::Group::serialize_elem(t3);

    let elem_len = <CS::Group as Group>::ElemLen::U16.to_be_bytes();

    // h2Input = I2OSP(len(Bm), 2) || Bm ||
    //           I2OSP(len(a0), 2) || a0 ||
    //           I2OSP(len(a1), 2) || a1 ||
    //           I2OSP(len(a2), 2) || a2 ||
    //           I2OSP(len(a3), 2) || a3 ||
    //           "Challenge"
    let h2_input = [
        &elem_len,
        bm.as_slice(),
        &elem_len,
        &a0,
        &elem_len,
        &a1,
        &elem_len,
        &a2,
        &elem_len,
        &a3,
        &STR_CHALLENGE,
    ];

    let dst = Dst::new::<CS, _, _>(STR_HASH_TO_SCALAR, mode);
    // This can't fail, the size of the `input` is known.
    let c = CS::Group::hash_to_scalar::<CS::Hash>(&h2_input, &dst.as_dst()).unwrap();

    match c.ct_eq(&proof.c_scalar).into() {
        true => Ok(()),
        false => Err(Error::ProofVerification),
    }
}

type ComputeCompositesResult<CS> = (
    <<CS as CipherSuite>::Group as Group>::Elem,
    <<CS as CipherSuite>::Group as Group>::Elem,
);

/// Can only fail with [`Error::Batch`].
fn compute_composites<
    CS: CipherSuite,
    IC: Iterator<Item = <CS::Group as Group>::Elem> + ExactSizeIterator,
    ID: Iterator<Item = <CS::Group as Group>::Elem> + ExactSizeIterator,
>(
    k_option: Option<<CS::Group as Group>::Scalar>,
    b: <CS::Group as Group>::Elem,
    c_slice: IC,
    d_slice: ID,
    mode: Mode,
) -> Result<ComputeCompositesResult<CS>>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    // https://www.rfc-editor.org/rfc/rfc9497#section-2.2.1

    let elem_len = <CS::Group as Group>::ElemLen::U16.to_be_bytes();

    if c_slice.len() != d_slice.len() {
        return Err(Error::Batch);
    }

    let len = u16::try_from(c_slice.len()).map_err(|_| Error::Batch)?;

    // seedDST = "Seed-" || contextString
    let seed_dst = Dst::new::<CS, _, _>(STR_SEED, mode);

    // h1Input = I2OSP(len(Bm), 2) || Bm ||
    //           I2OSP(len(seedDST), 2) || seedDST
    // seed = Hash(h1Input)
    let seed = CS::Hash::new()
        .chain_update(elem_len)
        .chain_update(CS::Group::serialize_elem(b))
        .chain_update(seed_dst.i2osp_2())
        .chain_update_multi(&seed_dst.as_dst())
        .finalize();
    let seed_len = i2osp_2_array(&seed);

    let mut m = CS::Group::identity_elem();
    let mut z = CS::Group::identity_elem();

    for (i, (c, d)) in (0..len).zip(c_slice.zip(d_slice)) {
        // Ci = GG.SerializeElement(Cs[i])
        let ci = CS::Group::serialize_elem(c);
        // Di = GG.SerializeElement(Ds[i])
        let di = CS::Group::serialize_elem(d);
        // h2Input = I2OSP(len(seed), 2) || seed || I2OSP(i, 2) ||
        //           I2OSP(len(Ci), 2) || Ci ||
        //           I2OSP(len(Di), 2) || Di ||
        //           "Composite"
        let h2_input = [
            seed_len.as_slice(),
            &seed,
            &i.to_be_bytes(),
            &elem_len,
            &ci,
            &elem_len,
            &di,
            &STR_COMPOSITE,
        ];

        let dst = Dst::new::<CS, _, _>(STR_HASH_TO_SCALAR, mode);
        // This can't fail, the size of the `input` is known.
        let di = CS::Group::hash_to_scalar::<CS::Hash>(&h2_input, &dst.as_dst()).unwrap();
        m = c * &di + &m;
        z = match k_option {
            Some(_) => z,
            None => d * &di + &z,
        };
    }

    z = match k_option {
        Some(k) => m * &k,
        None => z,
    };

    Ok((m, z))
}

/////////////////////
// Inner Functions //
// =============== //
/////////////////////

/// Can only fail with [`Error::DeriveKeyPair`] and [`Error::Protocol`].
pub(crate) fn derive_key_internal<CS: CipherSuite>(
    seed: &[u8],
    info: &[u8],
    mode: Mode,
) -> Result<<CS::Group as Group>::Scalar, Error>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    let dst = Dst::new::<CS, _, _>(STR_DERIVE_KEYPAIR, mode);

    let info_len = i2osp_2(info.len()).map_err(|_| Error::DeriveKeyPair)?;

    for counter in 0_u8..=u8::MAX {
        // deriveInput = seed || I2OSP(len(info), 2) || info
        // skS = G.HashToScalar(deriveInput || I2OSP(counter, 1), DST = "DeriveKeyPair"
        // || contextString)
        let sk_s = CS::Group::hash_to_scalar::<CS::Hash>(
            &[seed, &info_len, info, &counter.to_be_bytes()],
            &dst.as_dst(),
        )
        .map_err(|_| Error::DeriveKeyPair)?;

        if !bool::from(CS::Group::is_zero_scalar(sk_s)) {
            return Ok(sk_s);
        }
    }

    Err(Error::Protocol)
}

/// Corresponds to DeriveKeyPair() function from the VOPRF specification.
///
/// # Errors
/// - [`Error::DeriveKeyPair`] if the `input` and `seed` together are longer
///   then `u16::MAX - 3`.
/// - [`Error::Protocol`] if the protocol fails and can't be completed.
#[cfg(feature = "danger")]
pub fn derive_key<CS: CipherSuite>(
    seed: &[u8],
    info: &[u8],
    mode: Mode,
) -> Result<<CS::Group as Group>::Scalar, Error>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    derive_key_internal::<CS>(seed, info, mode)
}

type DeriveKeypairResult<CS> = (
    <<CS as CipherSuite>::Group as Group>::Scalar,
    <<CS as CipherSuite>::Group as Group>::Elem,
);

/// Can only fail with [`Error::DeriveKeyPair`] and [`Error::Protocol`].
pub(crate) fn derive_keypair<CS: CipherSuite>(
    seed: &[u8],
    info: &[u8],
    mode: Mode,
) -> Result<DeriveKeypairResult<CS>, Error>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    let sk_s = derive_key_internal::<CS>(seed, info, mode)?;
    let pk_s = CS::Group::base_elem() * &sk_s;

    Ok((sk_s, pk_s))
}

/// Inner function for blind that assumes that the blinding factor has already
/// been chosen, and therefore takes it as input. Does not check if the blinding
/// factor is non-zero.
///
/// Can only fail with [`Error::Input`].
pub(crate) fn deterministic_blind_unchecked<CS: CipherSuite>(
    input: &[u8],
    blind: &<CS::Group as Group>::Scalar,
    mode: Mode,
) -> Result<<CS::Group as Group>::Elem>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    let hashed_point = hash_to_group::<CS>(input, mode)?;
    Ok(hashed_point * blind)
}

/// Hashes `input` to a point on the curve
pub(crate) fn hash_to_group<CS: CipherSuite>(
    input: &[u8],
    mode: Mode,
) -> Result<<CS::Group as Group>::Elem>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    let dst = Dst::new::<CS, _, _>(STR_HASH_TO_GROUP, mode);
    CS::Group::hash_to_curve::<CS::Hash>(&[input], &dst.as_dst()).map_err(|_| Error::Input)
}

/// Internal function that finalizes the hash input for OPRF, VOPRF & POPRF.
/// Returned values can only fail with [`Error::Input`].
pub(crate) fn server_evaluate_hash_input<CS: CipherSuite>(
    input: &[u8],
    info: Option<&[u8]>,
    issued_element: GenericArray<u8, <<CS as CipherSuite>::Group as Group>::ElemLen>,
) -> Result<Output<CS::Hash>>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    // OPRF & VOPRF
    // hashInput = I2OSP(len(input), 2) || input ||
    //             I2OSP(len(issuedElement), 2) || issuedElement ||
    //             "Finalize"
    // return Hash(hashInput)
    //
    // POPRF
    // hashInput = I2OSP(len(input), 2) || input ||
    //             I2OSP(len(info), 2) || info ||
    //             I2OSP(len(issuedElement), 2) || issuedElement ||
    //             "Finalize"

    let mut hash = CS::Hash::new()
        .chain_update(i2osp_2(input.as_ref().len()).map_err(|_| Error::Input)?)
        .chain_update(input.as_ref());
    if let Some(info) = info {
        hash = hash
            .chain_update(i2osp_2(info.as_ref().len()).map_err(|_| Error::Input)?)
            .chain_update(info.as_ref());
    }
    Ok(hash
        .chain_update(i2osp_2(issued_element.as_ref().len()).map_err(|_| Error::Input)?)
        .chain_update(issued_element)
        .chain_update(STR_FINALIZE)
        .finalize())
}

pub(crate) struct Dst<L: ArrayLength<u8>> {
    dst_1: GenericArray<u8, L>,
    dst_2: &'static str,
}

impl<L: ArrayLength<u8>> Dst<L> {
    pub(crate) fn new<CS: CipherSuite, T, TL: ArrayLength<u8>>(par_1: T, mode: Mode) -> Self
    where
        T: Into<GenericArray<u8, TL>>,
        TL: Add<U9, Output = L>,
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let par_1 = par_1.into();
        // Generates the contextString parameter as defined in
        // <https://www.rfc-editor.org/rfc/rfc9497#section-3.1>
        let par_2 = GenericArray::from(STR_OPRF)
            .concat([mode.to_u8()].into())
            .concat([b'-'].into());

        let dst_1 = par_1.concat(par_2);
        let dst_2 = CS::ID;

        assert!(
            L::USIZE + dst_2.len() <= u16::MAX.into(),
            "constructed DST longer then {}",
            u16::MAX
        );

        Self { dst_1, dst_2 }
    }

    pub(crate) fn as_dst(&self) -> [&[u8]; 2] {
        [&self.dst_1, self.dst_2.as_bytes()]
    }

    pub(crate) fn i2osp_2(&self) -> [u8; 2] {
        u16::try_from(L::USIZE + self.dst_2.len())
            .unwrap()
            .to_be_bytes()
    }
}

trait DigestExt {
    fn chain_update_multi(self, data: &[&[u8]]) -> Self;
}

impl<T> DigestExt for T
where
    T: Digest,
{
    fn chain_update_multi(mut self, datas: &[&[u8]]) -> Self {
        for data in datas {
            self.update(data)
        }

        self
    }
}

///////////////////////
// Utility Functions //
// ================= //
///////////////////////

pub(crate) fn i2osp_2(input: usize) -> Result<[u8; 2], InternalError> {
    u16::try_from(input)
        .map(|input| input.to_be_bytes())
        .map_err(|_| InternalError::I2osp)
}

pub(crate) fn i2osp_2_array<L: ArrayLength<u8> + IsLess<U256>>(
    _: &GenericArray<u8, L>,
) -> GenericArray<u8, U2> {
    L::U16.to_be_bytes().into()
}
