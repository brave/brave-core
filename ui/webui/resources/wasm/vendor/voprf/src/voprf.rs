// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Contains the main VOPRF API

#[cfg(feature = "alloc")]
use alloc::vec::Vec;
use core::iter::{self, Map, Repeat, Zip};

use derive_where::derive_where;
use digest::core_api::BlockSizeUser;
use digest::{Digest, Output, OutputSizeUser};
use generic_array::typenum::{IsLess, IsLessOrEqual, Unsigned, U256};
use generic_array::GenericArray;
use rand_core::{CryptoRng, RngCore};

use crate::common::{
    derive_keypair, deterministic_blind_unchecked, generate_proof, hash_to_group, i2osp_2,
    server_evaluate_hash_input, verify_proof, BlindedElement, EvaluationElement, Mode,
    PreparedEvaluationElement, Proof, STR_FINALIZE,
};
#[cfg(feature = "serde")]
use crate::serialization::serde::{Element, Scalar};
use crate::{CipherSuite, Error, Group, Result};

////////////////////////////
// High-level API Structs //
// ====================== //
////////////////////////////

/// A client which engages with a [VoprfServer] in verifiable mode, meaning
/// that the OPRF outputs can be checked against a server public key.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct VoprfClient<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    #[cfg_attr(feature = "serde", serde(with = "Scalar::<CS::Group>"))]
    pub(crate) blind: <CS::Group as Group>::Scalar,
    #[cfg_attr(feature = "serde", serde(with = "Element::<CS::Group>"))]
    pub(crate) blinded_element: <CS::Group as Group>::Elem,
}

/// A server which engages with a [VoprfClient] in verifiable mode, meaning
/// that the OPRF outputs can be checked against a server public key.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct VoprfServer<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    #[cfg_attr(feature = "serde", serde(with = "Scalar::<CS::Group>"))]
    pub(crate) sk: <CS::Group as Group>::Scalar,
    #[cfg_attr(feature = "serde", serde(with = "Element::<CS::Group>"))]
    pub(crate) pk: <CS::Group as Group>::Elem,
}

/////////////////////////
// API Implementations //
// =================== //
/////////////////////////

impl<CS: CipherSuite> VoprfClient<CS>
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
    ) -> Result<VoprfClientBlindResult<CS>> {
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
    ) -> Result<VoprfClientBlindResult<CS>> {
        Self::deterministic_blind_unchecked_inner(input, blind)
    }

    /// Can only fail with [`Error::Input`].
    fn deterministic_blind_unchecked_inner(
        input: &[u8],
        blind: <CS::Group as Group>::Scalar,
    ) -> Result<VoprfClientBlindResult<CS>> {
        let blinded_element = deterministic_blind_unchecked::<CS>(input, &blind, Mode::Voprf)?;
        Ok(VoprfClientBlindResult {
            state: Self {
                blind,
                blinded_element,
            },
            message: BlindedElement(blinded_element),
        })
    }

    /// Computes the third step for the multiplicative blinding version of
    /// DH-OPRF, in which the client unblinds the server's message.
    ///
    /// # Errors
    /// - [`Error::Input`] if the `input` is empty or longer then [`u16::MAX`].
    /// - [`Error::ProofVerification`] if the `proof` failed to verify.
    pub fn finalize(
        &self,
        input: &[u8],
        evaluation_element: &EvaluationElement<CS>,
        proof: &Proof<CS>,
        pk: <CS::Group as Group>::Elem,
    ) -> Result<Output<CS::Hash>> {
        let inputs = core::array::from_ref(&input);
        let clients = core::array::from_ref(self);
        let messages = core::array::from_ref(evaluation_element);

        let mut batch_result = Self::batch_finalize(inputs, clients, messages, proof, pk)?;
        batch_result.next().unwrap()
    }

    /// Allows for batching of the finalization of multiple [VoprfClient]
    /// and [EvaluationElement] pairs
    ///
    /// # Errors
    /// - [`Error::Batch`] if the number of `clients` and `messages` don't match
    ///   or is longer then [`u16::MAX`].
    /// - [`Error::ProofVerification`] if the `proof` failed to verify.
    ///
    /// The resulting messages can each fail individually with [`Error::Input`]
    /// if the `input` is empty or longer then [`u16::MAX`].
    pub fn batch_finalize<'a, I: 'a, II, IC, IM>(
        inputs: &'a II,
        clients: &'a IC,
        messages: &'a IM,
        proof: &Proof<CS>,
        pk: <CS::Group as Group>::Elem,
    ) -> Result<VoprfClientBatchFinalizeResult<'a, CS, I, II, IC, IM>>
    where
        CS: 'a,
        I: AsRef<[u8]>,
        &'a II: 'a + IntoIterator<Item = I>,
        <&'a II as IntoIterator>::IntoIter: ExactSizeIterator,
        &'a IC: 'a + IntoIterator<Item = &'a VoprfClient<CS>>,
        <&'a IC as IntoIterator>::IntoIter: ExactSizeIterator,
        &'a IM: 'a + IntoIterator<Item = &'a EvaluationElement<CS>>,
        <&'a IM as IntoIterator>::IntoIter: ExactSizeIterator,
    {
        let unblinded_elements = verifiable_unblind(clients, messages, pk, proof)?;
        let inputs_and_unblinded_elements = inputs.into_iter().zip(unblinded_elements);
        Ok(finalize_after_unblind::<CS, _, _>(
            inputs_and_unblinded_elements,
        ))
    }

    /// Only used for test functions
    #[cfg(test)]
    pub fn from_blind_and_element(
        blind: <CS::Group as Group>::Scalar,
        blinded_element: <CS::Group as Group>::Elem,
    ) -> Self {
        Self {
            blind,
            blinded_element,
        }
    }

    // Only used for test functions
    #[cfg(test)]
    pub fn get_blind(&self) -> <CS::Group as Group>::Scalar {
        self.blind
    }
}

impl<CS: CipherSuite> VoprfServer<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Produces a new instance of a [VoprfServer] using a supplied RNG
    ///
    /// # Errors
    /// [`Error::Protocol`] if the protocol fails and can't be completed.
    pub fn new<R: RngCore + CryptoRng>(rng: &mut R) -> Result<Self> {
        let mut seed = GenericArray::<_, <CS::Group as Group>::ScalarLen>::default();
        rng.fill_bytes(&mut seed);
        // This can't fail as the hash output is type constrained.
        Self::new_from_seed(&seed, &[])
    }

    /// Produces a new instance of a [VoprfServer] using a supplied set of
    /// bytes to represent the server's private key
    ///
    /// # Errors
    /// [`Error::Deserialization`] if the private key is not a valid point on
    /// the group or zero.
    pub fn new_with_key(key: &[u8]) -> Result<Self> {
        let sk = CS::Group::deserialize_scalar(key)?;
        let pk = CS::Group::base_elem() * &sk;
        Ok(Self { sk, pk })
    }

    /// Produces a new instance of a [VoprfServer] using a supplied set of
    /// bytes which are used as a seed to derive the server's private key.
    ///
    /// Corresponds to DeriveKeyPair() function from the VOPRF specification.
    ///
    /// # Errors
    /// - [`Error::DeriveKeyPair`] if the `input` and `seed` together are longer
    ///   then `u16::MAX - 3`.
    /// - [`Error::Protocol`] if the protocol fails and can't be completed.
    pub fn new_from_seed(seed: &[u8], info: &[u8]) -> Result<Self> {
        let (sk, pk) = derive_keypair::<CS>(seed, info, Mode::Voprf)?;
        Ok(Self { sk, pk })
    }

    // Only used for tests
    #[cfg(test)]
    pub fn get_private_key(&self) -> <CS::Group as Group>::Scalar {
        self.sk
    }

    /// Computes the second step for the multiplicative blinding version of
    /// DH-OPRF. This message is sent from the server (who holds the OPRF key)
    /// to the client.
    pub fn blind_evaluate<R: RngCore + CryptoRng>(
        &self,
        rng: &mut R,
        blinded_element: &BlindedElement<CS>,
    ) -> VoprfServerEvaluateResult<CS> {
        let mut prepared_evaluation_elements =
            self.batch_blind_evaluate_prepare(iter::once(blinded_element));
        let prepared_evaluation_element = [prepared_evaluation_elements.next().unwrap()];

        // This can't fail because we know the size of the inputs.
        let VoprfServerBatchEvaluateFinishResult {
            mut messages,
            proof,
        } = self
            .batch_blind_evaluate_finish(
                rng,
                iter::once(blinded_element),
                &prepared_evaluation_element,
            )
            .unwrap();

        let message = messages.next().unwrap();

        VoprfServerEvaluateResult { message, proof }
    }

    /// Allows for batching of the evaluation of multiple [BlindedElement]
    /// messages from a [VoprfClient]
    ///
    /// # Errors
    /// [`Error::Batch`] if the number of `blinded_elements` and
    /// `evaluation_elements` don't match or is longer then [`u16::MAX`]
    #[cfg(feature = "alloc")]
    pub fn batch_blind_evaluate<'a, R: RngCore + CryptoRng, I>(
        &self,
        rng: &mut R,
        blinded_elements: &'a I,
    ) -> Result<VoprfServerBatchEvaluateResult<CS>>
    where
        CS: 'a,
        &'a I: IntoIterator<Item = &'a BlindedElement<CS>>,
        <&'a I as IntoIterator>::IntoIter: ExactSizeIterator,
    {
        let prepared_evaluation_elements = self
            .batch_blind_evaluate_prepare(blinded_elements.into_iter())
            .collect();
        let VoprfServerBatchEvaluateFinishResult { messages, proof } = self
            .batch_blind_evaluate_finish::<_, _, Vec<_>>(
                rng,
                blinded_elements.into_iter(),
                &prepared_evaluation_elements,
            )?;
        let messages = messages.collect();

        Ok(VoprfServerBatchEvaluateResult { messages, proof })
    }

    /// Alternative version of `batch_blind_evaluate` without memory allocation.
    /// Returned [`PreparedEvaluationElement`] have to be
    /// [`collect`](Iterator::collect)ed and passed into
    /// [`batch_blind_evaluate_finish`](Self::batch_blind_evaluate_finish).
    pub fn batch_blind_evaluate_prepare<'a, I: Iterator<Item = &'a BlindedElement<CS>>>(
        &self,
        blinded_elements: I,
    ) -> VoprfServerBatchEvaluatePreparedEvaluationElements<CS, I>
    where
        CS: 'a,
    {
        blinded_elements
            .zip(iter::repeat(self.sk))
            .map(|(blinded_element, sk)| {
                PreparedEvaluationElement(EvaluationElement(blinded_element.0 * &sk))
            })
    }

    /// See [`batch_blind_evaluate_prepare`](Self::batch_blind_evaluate_prepare)
    /// for more details.
    ///
    /// # Errors
    /// [`Error::Batch`] if the number of `blinded_elements` and
    /// `evaluation_elements` don't match or is longer then [`u16::MAX`]
    pub fn batch_blind_evaluate_finish<
        'a,
        'b,
        R: RngCore + CryptoRng,
        IB: Iterator<Item = &'a BlindedElement<CS>> + ExactSizeIterator,
        IE,
    >(
        &self,
        rng: &mut R,
        blinded_elements: IB,
        evaluation_elements: &'b IE,
    ) -> Result<VoprfServerBatchEvaluateFinishResult<'b, CS, IE>>
    where
        CS: 'a + 'b,
        &'b IE: IntoIterator<Item = &'b PreparedEvaluationElement<CS>>,
        <&'b IE as IntoIterator>::IntoIter: ExactSizeIterator,
    {
        let g = CS::Group::base_elem();
        let proof = generate_proof(
            rng,
            self.sk,
            g,
            self.pk,
            blinded_elements.map(|element| element.0),
            evaluation_elements.into_iter().map(|element| element.0 .0),
            Mode::Voprf,
        )?;

        let messages = evaluation_elements.into_iter().map(<fn(
            &PreparedEvaluationElement<CS>,
        ) -> EvaluationElement<CS>>::from(
            |element| EvaluationElement(element.0 .0),
        ));

        Ok(VoprfServerBatchEvaluateFinishResult { messages, proof })
    }

    /// Computes the output of the POPRF on the server side
    ///
    /// # Errors
    /// [`Error::Input`]  if the `input` is longer then [`u16::MAX`].
    pub fn evaluate(&self, input: &[u8]) -> Result<Output<<CS as CipherSuite>::Hash>> {
        let input_element = hash_to_group::<CS>(input, Mode::Voprf)?;
        if CS::Group::is_identity_elem(input_element).into() {
            return Err(Error::Input);
        };
        let evaluated_element = input_element * &self.sk;

        let issued_element = CS::Group::serialize_elem(evaluated_element);

        server_evaluate_hash_input::<CS>(input, None, issued_element)
    }

    /// Retrieves the server's public key
    pub fn get_public_key(&self) -> <CS::Group as Group>::Elem {
        self.pk
    }
}

/////////////////////////
// Convenience Structs //
//==================== //
/////////////////////////

/// Contains the fields that are returned by a verifiable client blind
#[derive_where(Debug; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
pub struct VoprfClientBlindResult<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// The state to be persisted on the client
    pub state: VoprfClient<CS>,
    /// The message to send to the server
    pub message: BlindedElement<CS>,
}

/// Concrete return type for [`VoprfClient::batch_finalize`].
pub type VoprfClientBatchFinalizeResult<'a, C, I, II, IC, IM> = FinalizeAfterUnblindResult<
    'a,
    C,
    I,
    Zip<<&'a II as IntoIterator>::IntoIter, VoprfUnblindResult<'a, C, IC, IM>>,
>;

/// Contains the fields that are returned by a verifiable server evaluate
#[derive_where(Debug; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
pub struct VoprfServerEvaluateResult<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// The message to send to the client
    pub message: EvaluationElement<CS>,
    /// The proof for the client to verify
    pub proof: Proof<CS>,
}

/// Contains the fields that are returned by a verifiable server batch evaluate
#[derive_where(Debug; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
#[cfg(feature = "alloc")]
pub struct VoprfServerBatchEvaluateResult<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// The messages to send to the client
    pub messages: Vec<EvaluationElement<CS>>,
    /// The proof for the client to verify
    pub proof: Proof<CS>,
}

/// Concrete type of [`EvaluationElement`]s returned by
/// [`VoprfServer::batch_blind_evaluate_prepare`].
pub type VoprfServerBatchEvaluatePreparedEvaluationElements<CS, I> = Map<
    Zip<I, Repeat<<<CS as CipherSuite>::Group as Group>::Scalar>>,
    fn(
        (
            &BlindedElement<CS>,
            <<CS as CipherSuite>::Group as Group>::Scalar,
        ),
    ) -> PreparedEvaluationElement<CS>,
>;

/// Concrete type of [`EvaluationElement`]s in
/// [`VoprfServerBatchEvaluateFinishResult`].
pub type VoprfServerBatchEvaluateFinishedMessages<'a, CS, I> = Map<
    <&'a I as IntoIterator>::IntoIter,
    fn(&PreparedEvaluationElement<CS>) -> EvaluationElement<CS>,
>;

/// Contains the fields that are returned by a verifiable server batch evaluate
/// finish.
#[derive_where(Debug; <&'a I as IntoIterator>::IntoIter, <CS::Group as Group>::Scalar)]
pub struct VoprfServerBatchEvaluateFinishResult<'a, CS: 'a + CipherSuite, I>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    &'a I: IntoIterator<Item = &'a PreparedEvaluationElement<CS>>,
{
    /// The [`EvaluationElement`]s to send to the client
    pub messages: VoprfServerBatchEvaluateFinishedMessages<'a, CS, I>,
    /// The proof for the client to verify
    pub proof: Proof<CS>,
}

/////////////////////
// Inner functions //
// =============== //
/////////////////////

type VoprfUnblindResult<'a, CS, IC, IM> = Map<
    Zip<
        Map<
            <&'a IC as IntoIterator>::IntoIter,
            fn(&VoprfClient<CS>) -> <<CS as CipherSuite>::Group as Group>::Scalar,
        >,
        <&'a IM as IntoIterator>::IntoIter,
    >,
    fn(
        (
            <<CS as CipherSuite>::Group as Group>::Scalar,
            &EvaluationElement<CS>,
        ),
    ) -> <<CS as CipherSuite>::Group as Group>::Elem,
>;

/// Can only fail with [`Error::Batch] or [`Error::ProofVerification`].
fn verifiable_unblind<'a, CS: 'a + CipherSuite, IC, IM>(
    clients: &'a IC,
    messages: &'a IM,
    pk: <CS::Group as Group>::Elem,
    proof: &Proof<CS>,
) -> Result<VoprfUnblindResult<'a, CS, IC, IM>>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    &'a IC: 'a + IntoIterator<Item = &'a VoprfClient<CS>>,
    <&'a IC as IntoIterator>::IntoIter: ExactSizeIterator,
    &'a IM: 'a + IntoIterator<Item = &'a EvaluationElement<CS>>,
    <&'a IM as IntoIterator>::IntoIter: ExactSizeIterator,
{
    let g = CS::Group::base_elem();

    let blinds = clients
        .into_iter()
        // Convert to `fn` pointer to make a return type possible.
        .map(<fn(&VoprfClient<CS>) -> _>::from(|x| x.blind));
    let evaluation_elements = messages.into_iter().map(|element| element.0);
    let blinded_elements = clients.into_iter().map(|client| client.blinded_element);

    verify_proof(
        g,
        pk,
        blinded_elements,
        evaluation_elements,
        proof,
        Mode::Voprf,
    )?;

    Ok(blinds
        .zip(messages)
        .map(|(blind, x)| x.0 * &CS::Group::invert_scalar(blind)))
}

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
) -> FinalizeAfterUnblindResult<'a, CS, I, IE>
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
    use core::ops::Add;
    use core::ptr;

    use ::alloc::vec;
    use ::alloc::vec::Vec;
    use generic_array::typenum::Sum;
    use generic_array::ArrayLength;
    use rand::rngs::OsRng;

    use super::*;
    use crate::common::{Dst, STR_HASH_TO_GROUP};
    use crate::Group;

    fn prf<CS: CipherSuite>(
        input: &[u8],
        key: <CS::Group as Group>::Scalar,
        mode: Mode,
    ) -> Output<CS::Hash>
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let dst = Dst::new::<CS, _, _>(STR_HASH_TO_GROUP, mode);
        let point = CS::Group::hash_to_curve::<CS::Hash>(&[input], &dst.as_dst()).unwrap();

        let res = point * &key;

        finalize_after_unblind::<CS, _, _>(iter::once((input, res)))
            .next()
            .unwrap()
            .unwrap()
    }

    fn verifiable_retrieval<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = VoprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server = VoprfServer::<CS>::new(&mut rng).unwrap();
        let server_result = server.blind_evaluate(&mut rng, &client_blind_result.message);
        let client_finalize_result = client_blind_result
            .state
            .finalize(
                input,
                &server_result.message,
                &server_result.proof,
                server.get_public_key(),
            )
            .unwrap();
        let res2 = prf::<CS>(input, server.get_private_key(), Mode::Voprf);
        assert_eq!(client_finalize_result, res2);
    }

    fn verifiable_batch_retrieval<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let mut rng = OsRng;
        let mut inputs = vec![];
        let mut client_states = vec![];
        let mut client_messages = vec![];
        let num_iterations = 10;
        for _ in 0..num_iterations {
            let mut input = [0u8; 32];
            rng.fill_bytes(&mut input);
            let client_blind_result = VoprfClient::<CS>::blind(&input, &mut rng).unwrap();
            inputs.push(input);
            client_states.push(client_blind_result.state);
            client_messages.push(client_blind_result.message);
        }
        let server = VoprfServer::<CS>::new(&mut rng).unwrap();
        let prepared_evaluation_elements: Vec<_> = server
            .batch_blind_evaluate_prepare(client_messages.iter())
            .collect();
        let VoprfServerBatchEvaluateFinishResult { messages, proof } = server
            .batch_blind_evaluate_finish(
                &mut rng,
                client_messages.iter(),
                &prepared_evaluation_elements,
            )
            .unwrap();
        let messages: Vec<_> = messages.collect();
        let client_finalize_result = VoprfClient::batch_finalize(
            &inputs,
            &client_states,
            &messages,
            &proof,
            server.get_public_key(),
        )
        .unwrap()
        .collect::<Result<Vec<_>>>()
        .unwrap();
        let mut res2 = vec![];
        for input in inputs.iter().take(num_iterations) {
            let output = prf::<CS>(input, server.get_private_key(), Mode::Voprf);
            res2.push(output);
        }
        assert_eq!(client_finalize_result, res2);
    }

    fn verifiable_batch_bad_public_key<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let mut rng = OsRng;
        let mut inputs = vec![];
        let mut client_states = vec![];
        let mut client_messages = vec![];
        let num_iterations = 10;
        for _ in 0..num_iterations {
            let mut input = [0u8; 32];
            rng.fill_bytes(&mut input);
            let client_blind_result = VoprfClient::<CS>::blind(&input, &mut rng).unwrap();
            inputs.push(input);
            client_states.push(client_blind_result.state);
            client_messages.push(client_blind_result.message);
        }
        let server = VoprfServer::<CS>::new(&mut rng).unwrap();
        let prepared_evaluation_elements: Vec<_> = server
            .batch_blind_evaluate_prepare(client_messages.iter())
            .collect();
        let VoprfServerBatchEvaluateFinishResult { messages, proof } = server
            .batch_blind_evaluate_finish(
                &mut rng,
                client_messages.iter(),
                &prepared_evaluation_elements,
            )
            .unwrap();
        let messages: Vec<_> = messages.collect();
        let wrong_pk = {
            let dst = Dst::new::<CS, _, _>(STR_HASH_TO_GROUP, Mode::Oprf);
            // Choose a group element that is unlikely to be the right public key
            CS::Group::hash_to_curve::<CS::Hash>(&[b"msg"], &dst.as_dst()).unwrap()
        };
        let client_finalize_result =
            VoprfClient::batch_finalize(&inputs, &client_states, &messages, &proof, wrong_pk);
        assert!(client_finalize_result.is_err());
    }

    fn verifiable_bad_public_key<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = VoprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server = VoprfServer::<CS>::new(&mut rng).unwrap();
        let server_result = server.blind_evaluate(&mut rng, &client_blind_result.message);
        let wrong_pk = {
            let dst = Dst::new::<CS, _, _>(STR_HASH_TO_GROUP, Mode::Oprf);
            // Choose a group element that is unlikely to be the right public key
            CS::Group::hash_to_curve::<CS::Hash>(&[b"msg"], &dst.as_dst()).unwrap()
        };
        let client_finalize_result = client_blind_result.state.finalize(
            input,
            &server_result.message,
            &server_result.proof,
            wrong_pk,
        );
        assert!(client_finalize_result.is_err());
    }

    fn verifiable_server_evaluate<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = VoprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server = VoprfServer::<CS>::new(&mut rng).unwrap();
        let server_result = server.blind_evaluate(&mut rng, &client_blind_result.message);

        let client_finalize = client_blind_result
            .state
            .finalize(
                input,
                &server_result.message,
                &server_result.proof,
                server.get_public_key(),
            )
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

    fn zeroize_voprf_client<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ElemLen>,
        Sum<<CS::Group as Group>::ScalarLen, <CS::Group as Group>::ElemLen>: ArrayLength<u8>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = VoprfClient::<CS>::blind(input, &mut rng).unwrap();

        let mut state = client_blind_result.state;
        unsafe { ptr::drop_in_place(&mut state) };
        assert!(state.serialize().iter().all(|&x| x == 0));

        let mut message = client_blind_result.message;
        unsafe { ptr::drop_in_place(&mut message) };
        assert!(message.serialize().iter().all(|&x| x == 0));
    }

    fn zeroize_voprf_server<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ElemLen>,
        Sum<<CS::Group as Group>::ScalarLen, <CS::Group as Group>::ElemLen>: ArrayLength<u8>,
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ScalarLen>,
        Sum<<CS::Group as Group>::ScalarLen, <CS::Group as Group>::ScalarLen>: ArrayLength<u8>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = VoprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server = VoprfServer::<CS>::new(&mut rng).unwrap();
        let server_result = server.blind_evaluate(&mut rng, &client_blind_result.message);

        let mut state = server;
        unsafe { ptr::drop_in_place(&mut state) };
        assert!(state.serialize().iter().all(|&x| x == 0));

        let mut message = server_result.message;
        unsafe { ptr::drop_in_place(&mut message) };
        assert!(message.serialize().iter().all(|&x| x == 0));

        let mut proof = server_result.proof;
        unsafe { ptr::drop_in_place(&mut proof) };
        assert!(proof.serialize().iter().all(|&x| x == 0));
    }

    #[test]
    fn test_functionality() -> Result<()> {
        use p256::NistP256;
        use p384::NistP384;
        use p521::NistP521;

        #[cfg(feature = "ristretto255")]
        {
            use crate::Ristretto255;

            verifiable_retrieval::<Ristretto255>();
            verifiable_batch_retrieval::<Ristretto255>();
            verifiable_bad_public_key::<Ristretto255>();
            verifiable_batch_bad_public_key::<Ristretto255>();
            verifiable_server_evaluate::<Ristretto255>();

            zeroize_voprf_client::<Ristretto255>();
            zeroize_voprf_server::<Ristretto255>();
        }

        verifiable_retrieval::<NistP256>();
        verifiable_batch_retrieval::<NistP256>();
        verifiable_bad_public_key::<NistP256>();
        verifiable_batch_bad_public_key::<NistP256>();
        verifiable_server_evaluate::<NistP256>();

        zeroize_voprf_client::<NistP256>();
        zeroize_voprf_server::<NistP256>();

        verifiable_retrieval::<NistP384>();
        verifiable_batch_retrieval::<NistP384>();
        verifiable_bad_public_key::<NistP384>();
        verifiable_batch_bad_public_key::<NistP384>();
        verifiable_server_evaluate::<NistP384>();

        zeroize_voprf_client::<NistP384>();
        zeroize_voprf_server::<NistP384>();

        verifiable_retrieval::<NistP521>();
        verifiable_batch_retrieval::<NistP521>();
        verifiable_bad_public_key::<NistP521>();
        verifiable_batch_bad_public_key::<NistP521>();
        verifiable_server_evaluate::<NistP521>();

        zeroize_voprf_client::<NistP521>();
        zeroize_voprf_server::<NistP521>();

        Ok(())
    }
}
