// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Contains the main OPRF API

use core::iter::{self, Map};

use derive_where::derive_where;
use digest::core_api::BlockSizeUser;
use digest::{Digest, Output, OutputSizeUser};
use generic_array::typenum::{IsLess, IsLessOrEqual, Unsigned, U256};
use generic_array::GenericArray;
use rand_core::{CryptoRng, RngCore};

use crate::common::{
    derive_key_internal, deterministic_blind_unchecked, hash_to_group, i2osp_2,
    server_evaluate_hash_input, BlindedElement, EvaluationElement, Mode, STR_FINALIZE,
};
#[cfg(feature = "serde")]
use crate::serialization::serde::Scalar;
use crate::{CipherSuite, Error, Group, Result};

///////////////
// Constants //
// ========= //
///////////////

////////////////////////////
// High-level API Structs //
// ====================== //
////////////////////////////

/// A client which engages with a [OprfServer] in base mode, meaning
/// that the OPRF outputs are not verifiable.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Scalar)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct OprfClient<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    #[cfg_attr(feature = "serde", serde(with = "Scalar::<CS::Group>"))]
    pub(crate) blind: <CS::Group as Group>::Scalar,
}

/// A server which engages with a [OprfClient] in base mode, meaning
/// that the OPRF outputs are not verifiable.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Scalar)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct OprfServer<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    #[cfg_attr(feature = "serde", serde(with = "Scalar::<CS::Group>"))]
    pub(crate) sk: <CS::Group as Group>::Scalar,
}

/////////////////////////
// API Implementations //
// =================== //
/////////////////////////

impl<CS: CipherSuite> OprfClient<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Computes the first step for the multiplicative blinding version of
    /// DH-OPRF.
    ///
    /// # Errors
    /// [`Error::Input`] if the `input` is empty or longer then [`u16::MAX`].
    pub fn blind<R: RngCore + CryptoRng>(
        input: &[u8],
        blinding_factor_rng: &mut R,
    ) -> Result<OprfClientBlindResult<CS>> {
        let blind = CS::Group::random_scalar(blinding_factor_rng);
        Self::deterministic_blind_unchecked_inner(input, blind)
    }

    /// Computes the first step for the multiplicative blinding version of
    /// DH-OPRF, taking a blinding factor scalar as input instead of sampling
    /// from an RNG.
    ///
    /// # Caution
    ///
    /// This should be used with caution, since it does not perform any checks
    /// on the validity of the blinding factor!
    ///
    /// # Errors
    /// [`Error::Input`] if the `input` is empty or longer then [`u16::MAX`].
    #[cfg(any(feature = "danger", test))]
    pub fn deterministic_blind_unchecked(
        input: &[u8],
        blind: <CS::Group as Group>::Scalar,
    ) -> Result<OprfClientBlindResult<CS>> {
        Self::deterministic_blind_unchecked_inner(input, blind)
    }

    /// Can only fail with [`Error::Input`].
    fn deterministic_blind_unchecked_inner(
        input: &[u8],
        blind: <CS::Group as Group>::Scalar,
    ) -> Result<OprfClientBlindResult<CS>> {
        let blinded_element = deterministic_blind_unchecked::<CS>(input, &blind, Mode::Oprf)?;
        Ok(OprfClientBlindResult {
            state: Self { blind },
            message: BlindedElement(blinded_element),
        })
    }

    /// Computes the third step for the multiplicative blinding version of
    /// DH-OPRF, in which the client unblinds the server's message.
    ///
    /// # Errors
    /// [`Error::Input`] if the `input` is empty or longer then [`u16::MAX`].
    pub fn finalize(
        &self,
        input: &[u8],
        evaluation_element: &EvaluationElement<CS>,
    ) -> Result<Output<CS::Hash>> {
        let unblinded_element = evaluation_element.0 * &CS::Group::invert_scalar(self.blind);
        let mut outputs =
            finalize_after_unblind::<CS, _, _>(iter::once((input, unblinded_element)), &[]);
        outputs.next().unwrap()
    }

    /// Only used for test functions
    #[cfg(test)]
    pub fn from_blind(blind: <CS::Group as Group>::Scalar) -> Self {
        Self { blind }
    }

    /// Exposes the blind group element
    #[cfg(feature = "danger")]
    pub fn get_blind(&self) -> <CS::Group as Group>::Scalar {
        self.blind
    }
}

impl<CS: CipherSuite> OprfServer<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Produces a new instance of a [OprfServer] using a supplied RNG
    ///
    /// # Errors
    /// [`Error::Protocol`] if the protocol fails and can't be completed.
    pub fn new<R: RngCore + CryptoRng>(rng: &mut R) -> Result<Self> {
        let mut seed = GenericArray::<_, <CS::Group as Group>::ScalarLen>::default();
        rng.fill_bytes(&mut seed);
        Self::new_from_seed(&seed, &[])
    }

    /// Produces a new instance of a [OprfServer] using a supplied set
    /// of bytes to represent the server's private key
    ///
    /// # Errors
    /// [`Error::Deserialization`] if the private key is not a valid point on
    /// the group or zero.
    pub fn new_with_key(private_key_bytes: &[u8]) -> Result<Self> {
        let sk = CS::Group::deserialize_scalar(private_key_bytes)?;
        Ok(Self { sk })
    }

    /// Produces a new instance of a [OprfServer] using a supplied set
    /// of bytes which are used as a seed to derive the server's private key.
    ///
    /// Corresponds to DeriveKeyPair() function from the VOPRF specification.
    ///
    /// # Errors
    /// - [`Error::DeriveKeyPair`] if the `input` and `seed` together are longer
    ///   then `u16::MAX - 3`.
    /// - [`Error::Protocol`] if the protocol fails and can't be completed.
    pub fn new_from_seed(seed: &[u8], info: &[u8]) -> Result<Self> {
        let sk = derive_key_internal::<CS>(seed, info, Mode::Oprf)?;
        Ok(Self { sk })
    }

    // Only used for tests
    #[cfg(test)]
    pub fn get_private_key(&self) -> <CS::Group as Group>::Scalar {
        self.sk
    }

    /// Computes the second step for the multiplicative blinding version of
    /// DH-OPRF. This message is sent from the server (who holds the OPRF key)
    /// to the client.
    pub fn blind_evaluate(&self, blinded_element: &BlindedElement<CS>) -> EvaluationElement<CS> {
        EvaluationElement(blinded_element.0 * &self.sk)
    }

    /// Computes the output of the OPRF on the server side
    ///
    /// # Errors
    /// [`Error::Input`]  if the `input` is longer then [`u16::MAX`].
    pub fn evaluate(&self, input: &[u8]) -> Result<Output<<CS as CipherSuite>::Hash>> {
        let input_element = hash_to_group::<CS>(input, Mode::Oprf)?;
        if CS::Group::is_identity_elem(input_element).into() {
            return Err(Error::Input);
        };
        let evaluated_element = input_element * &self.sk;

        let issued_element = CS::Group::serialize_elem(evaluated_element);

        server_evaluate_hash_input::<CS>(input, None, issued_element)
    }
}

/////////////////////////
// Convenience Structs //
//==================== //
/////////////////////////

/// Contains the fields that are returned by a non-verifiable client blind
#[derive_where(Debug; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
pub struct OprfClientBlindResult<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// The state to be persisted on the client
    pub state: OprfClient<CS>,
    /// The message to send to the server
    pub message: BlindedElement<CS>,
}

/////////////////////
// Inner functions //
// =============== //
/////////////////////

type FinalizeAfterUnblindResult<'a, C, I, IE> = Map<
    IE,
    fn((I, <<C as CipherSuite>::Group as Group>::Elem)) -> Result<Output<<C as CipherSuite>::Hash>>,
>;

/// Returned values can only fail with [`Error::Input`].
fn finalize_after_unblind<
    'a,
    CS: CipherSuite,
    I: AsRef<[u8]>,
    IE: 'a + Iterator<Item = (I, <CS::Group as Group>::Elem)>,
>(
    inputs_and_unblinded_elements: IE,
    _unused: &'a [u8],
) -> FinalizeAfterUnblindResult<CS, I, IE>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    inputs_and_unblinded_elements.map(|(input, unblinded_element)| {
        let elem_len = <CS::Group as Group>::ElemLen::U16.to_be_bytes();

        // hashInput = I2OSP(len(input), 2) || input ||
        //             I2OSP(len(unblindedElement), 2) || unblindedElement ||
        //             "Finalize"
        // return Hash(hashInput)
        Ok(CS::Hash::new()
            .chain_update(i2osp_2(input.as_ref().len()).map_err(|_| Error::Input)?)
            .chain_update(input.as_ref())
            .chain_update(elem_len)
            .chain_update(CS::Group::serialize_elem(unblinded_element))
            .chain_update(STR_FINALIZE)
            .finalize())
    })
}

///////////
// Tests //
// ===== //
///////////

#[cfg(test)]
mod tests {
    use core::ptr;

    use rand::rngs::OsRng;

    use super::*;
    use crate::common::{Dst, STR_HASH_TO_GROUP};
    use crate::Group;

    fn prf<CS: CipherSuite>(
        input: &[u8],
        key: <CS::Group as Group>::Scalar,
        info: &[u8],
        mode: Mode,
    ) -> Output<CS::Hash>
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let dst = Dst::new::<CS, _, _>(STR_HASH_TO_GROUP, mode);
        let point = CS::Group::hash_to_curve::<CS::Hash>(&[input], &dst.as_dst()).unwrap();

        let res = point * &key;

        finalize_after_unblind::<CS, _, _>(iter::once((input, res)), info)
            .next()
            .unwrap()
            .unwrap()
    }

    fn base_retrieval<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = OprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server = OprfServer::<CS>::new(&mut rng).unwrap();
        let message = server.blind_evaluate(&client_blind_result.message);
        let client_finalize_result = client_blind_result.state.finalize(input, &message).unwrap();
        let res2 = prf::<CS>(input, server.get_private_key(), &[], Mode::Oprf);
        assert_eq!(client_finalize_result, res2);
    }

    fn base_inversion_unsalted<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let mut rng = OsRng;
        let mut input = [0u8; 64];
        rng.fill_bytes(&mut input);
        let client_blind_result = OprfClient::<CS>::blind(&input, &mut rng).unwrap();
        let client_finalize_result = client_blind_result
            .state
            .finalize(&input, &EvaluationElement(client_blind_result.message.0))
            .unwrap();

        let dst = Dst::new::<CS, _, _>(STR_HASH_TO_GROUP, Mode::Oprf);
        let point = CS::Group::hash_to_curve::<CS::Hash>(&[&input], &dst.as_dst()).unwrap();
        let res2 = finalize_after_unblind::<CS, _, _>(iter::once((input.as_ref(), point)), &[])
            .next()
            .unwrap()
            .unwrap();

        assert_eq!(client_finalize_result, res2);
    }

    fn server_evaluate<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = OprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server = OprfServer::<CS>::new(&mut rng).unwrap();
        let server_result = server.blind_evaluate(&client_blind_result.message);

        let client_finalize = client_blind_result
            .state
            .finalize(input, &server_result)
            .unwrap();

        // We expect the outputs from client and server to be equal given an identical
        // input
        let server_evaluate = server.evaluate(input).unwrap();
        assert_eq!(client_finalize, server_evaluate);

        // We expect the outputs from client and server to be different given different
        // inputs
        let wrong_input = b"wrong input";
        let server_evaluate = server.evaluate(wrong_input).unwrap();
        assert!(client_finalize != server_evaluate);
    }

    fn zeroize_oprf_client<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = OprfClient::<CS>::blind(input, &mut rng).unwrap();

        let mut state = client_blind_result.state;
        unsafe { ptr::drop_in_place(&mut state) };
        assert!(state.serialize().iter().all(|&x| x == 0));

        let mut message = client_blind_result.message;
        unsafe { ptr::drop_in_place(&mut message) };
        assert!(message.serialize().iter().all(|&x| x == 0));
    }

    fn zeroize_oprf_server<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = OprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server = OprfServer::<CS>::new(&mut rng).unwrap();
        let mut message = server.blind_evaluate(&client_blind_result.message);

        let mut state = server;
        unsafe { ptr::drop_in_place(&mut state) };
        assert!(state.serialize().iter().all(|&x| x == 0));

        unsafe { ptr::drop_in_place(&mut message) };
        assert!(message.serialize().iter().all(|&x| x == 0));
    }

    #[test]
    fn test_functionality() -> Result<()> {
        use p256::NistP256;
        use p384::NistP384;
        use p521::NistP521;

        #[cfg(feature = "ristretto255")]
        {
            use crate::Ristretto255;

            base_retrieval::<Ristretto255>();
            base_inversion_unsalted::<Ristretto255>();
            server_evaluate::<Ristretto255>();

            zeroize_oprf_client::<Ristretto255>();
            zeroize_oprf_server::<Ristretto255>();
        }

        base_retrieval::<NistP256>();
        base_inversion_unsalted::<NistP256>();
        server_evaluate::<NistP256>();

        zeroize_oprf_client::<NistP256>();
        zeroize_oprf_server::<NistP256>();

        base_retrieval::<NistP384>();
        base_inversion_unsalted::<NistP384>();
        server_evaluate::<NistP384>();

        zeroize_oprf_client::<NistP384>();
        zeroize_oprf_server::<NistP384>();

        base_retrieval::<NistP521>();
        base_inversion_unsalted::<NistP521>();
        server_evaluate::<NistP521>();

        zeroize_oprf_client::<NistP521>();
        zeroize_oprf_server::<NistP521>();

        Ok(())
    }
}
