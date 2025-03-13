// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;
use core::ops::Add;

use digest::core_api::BlockSizeUser;
use digest::OutputSizeUser;
use generic_array::typenum::{IsLess, IsLessOrEqual, Sum, U256};
use generic_array::ArrayLength;
use serde_json::Value;

use crate::tests::mock_rng::CycleRng;
use crate::tests::parser::*;
use crate::{
    BlindedElement, CipherSuite, EvaluationElement, Group, OprfClient, OprfServer, PoprfClient,
    PoprfServer, PoprfServerBatchEvaluateFinishResult, PoprfServerBatchEvaluatePrepareResult,
    Proof, Result, VoprfClient, VoprfServer, VoprfServerBatchEvaluateFinishResult,
};

#[derive(Debug)]
struct VOPRFTestVectorParameters {
    seed: Vec<u8>,
    sksm: Vec<u8>,
    pksm: Vec<u8>,
    input: Vec<Vec<u8>>,
    info: Vec<u8>,
    key_info: Vec<u8>,
    blind: Vec<Vec<u8>>,
    blinded_element: Vec<Vec<u8>>,
    evaluation_element: Vec<Vec<u8>>,
    proof: Vec<u8>,
    proof_random_scalar: Vec<u8>,
    output: Vec<Vec<u8>>,
}

fn populate_test_vectors(values: &Value) -> VOPRFTestVectorParameters {
    VOPRFTestVectorParameters {
        seed: decode(values, "Seed"),
        sksm: decode(values, "skSm"),
        pksm: decode(values, "pkSm"),
        input: decode_vec(values, "Input"),
        info: decode(values, "Info"),
        key_info: decode(values, "KeyInfo"),
        blind: decode_vec(values, "Blind"),
        blinded_element: decode_vec(values, "BlindedElement"),
        evaluation_element: decode_vec(values, "EvaluationElement"),
        proof: decode(values, "Proof"),
        proof_random_scalar: decode(values, "ProofRandomScalar"),
        output: decode_vec(values, "Output"),
    }
}

fn decode(values: &Value, key: &str) -> Vec<u8> {
    values[key]
        .as_str()
        .and_then(|s| hex::decode(s).ok())
        .unwrap_or_default()
}

fn decode_vec(values: &Value, key: &str) -> Vec<Vec<u8>> {
    let s = values[key].as_str().unwrap();
    let res = match s.contains(',') {
        true => Some(s.split(',').map(|x| hex::decode(x).unwrap()).collect()),
        false => Some(vec![hex::decode(s).unwrap()]),
    };
    res.unwrap()
}

macro_rules! json_to_test_vectors {
    ( $v:ident, $cs:expr, $mode:expr ) => {
        $v[$cs][$mode]
            .as_array()
            .into_iter()
            .flatten()
            .map(populate_test_vectors)
            .collect::<Vec<VOPRFTestVectorParameters>>()
    };
}

#[test]
fn test_vectors() -> Result<()> {
    use p256::NistP256;
    use p384::NistP384;
    use p521::NistP521;

    let rfc: Value = serde_json::from_str(rfc_to_json(super::cfrg_vectors::VECTORS).as_str())
        .expect("Could not parse json");

    #[cfg(feature = "ristretto255")]
    {
        use crate::Ristretto255;

        let ristretto_oprf_tvs = json_to_test_vectors!(
            rfc,
            String::from("ristretto255-SHA512"),
            String::from("OPRF")
        );
        assert_ne!(ristretto_oprf_tvs.len(), 0);
        test_oprf_seed_to_key::<Ristretto255>(&ristretto_oprf_tvs)?;
        test_oprf_blind::<Ristretto255>(&ristretto_oprf_tvs)?;
        test_oprf_blind_evaluate::<Ristretto255>(&ristretto_oprf_tvs)?;
        test_oprf_finalize::<Ristretto255>(&ristretto_oprf_tvs)?;
        test_oprf_evaluate::<Ristretto255>(&ristretto_oprf_tvs)?;

        let ristretto_voprf_tvs = json_to_test_vectors!(
            rfc,
            String::from("ristretto255-SHA512"),
            String::from("VOPRF")
        );
        assert_ne!(ristretto_voprf_tvs.len(), 0);
        test_voprf_seed_to_key::<Ristretto255>(&ristretto_voprf_tvs)?;
        test_voprf_blind::<Ristretto255>(&ristretto_voprf_tvs)?;
        test_voprf_blind_evaluate::<Ristretto255>(&ristretto_voprf_tvs)?;
        test_voprf_finalize::<Ristretto255>(&ristretto_voprf_tvs)?;
        test_voprf_evaluate::<Ristretto255>(&ristretto_voprf_tvs)?;

        let ristretto_poprf_tvs = json_to_test_vectors!(
            rfc,
            String::from("ristretto255-SHA512"),
            String::from("POPRF")
        );
        assert_ne!(ristretto_poprf_tvs.len(), 0);
        test_poprf_seed_to_key::<Ristretto255>(&ristretto_poprf_tvs)?;
        test_poprf_blind::<Ristretto255>(&ristretto_poprf_tvs)?;
        test_poprf_blind_evaluate::<Ristretto255>(&ristretto_poprf_tvs)?;
        test_poprf_finalize::<Ristretto255>(&ristretto_poprf_tvs)?;
        test_poprf_evaluate::<Ristretto255>(&ristretto_poprf_tvs)?;
    }

    let p256_oprf_tvs =
        json_to_test_vectors!(rfc, String::from("P256-SHA256"), String::from("OPRF"));
    assert_ne!(p256_oprf_tvs.len(), 0);
    test_oprf_seed_to_key::<NistP256>(&p256_oprf_tvs)?;
    test_oprf_blind::<NistP256>(&p256_oprf_tvs)?;
    test_oprf_blind_evaluate::<NistP256>(&p256_oprf_tvs)?;
    test_oprf_finalize::<NistP256>(&p256_oprf_tvs)?;
    test_oprf_evaluate::<NistP256>(&p256_oprf_tvs)?;

    let p256_voprf_tvs =
        json_to_test_vectors!(rfc, String::from("P256-SHA256"), String::from("VOPRF"));
    assert_ne!(p256_voprf_tvs.len(), 0);
    test_voprf_seed_to_key::<NistP256>(&p256_voprf_tvs)?;
    test_voprf_blind::<NistP256>(&p256_voprf_tvs)?;
    test_voprf_blind_evaluate::<NistP256>(&p256_voprf_tvs)?;
    test_voprf_finalize::<NistP256>(&p256_voprf_tvs)?;
    test_voprf_evaluate::<NistP256>(&p256_voprf_tvs)?;

    let p256_poprf_tvs =
        json_to_test_vectors!(rfc, String::from("P256-SHA256"), String::from("POPRF"));
    assert_ne!(p256_poprf_tvs.len(), 0);
    test_poprf_seed_to_key::<NistP256>(&p256_poprf_tvs)?;
    test_poprf_blind::<NistP256>(&p256_poprf_tvs)?;
    test_poprf_blind_evaluate::<NistP256>(&p256_poprf_tvs)?;
    test_poprf_finalize::<NistP256>(&p256_poprf_tvs)?;
    test_poprf_evaluate::<NistP256>(&p256_poprf_tvs)?;

    let p384_oprf_tvs =
        json_to_test_vectors!(rfc, String::from("P384-SHA384"), String::from("OPRF"));
    assert_ne!(p384_oprf_tvs.len(), 0);
    test_oprf_seed_to_key::<NistP384>(&p384_oprf_tvs)?;
    test_oprf_blind::<NistP384>(&p384_oprf_tvs)?;
    test_oprf_blind_evaluate::<NistP384>(&p384_oprf_tvs)?;
    test_oprf_finalize::<NistP384>(&p384_oprf_tvs)?;
    test_oprf_evaluate::<NistP384>(&p384_oprf_tvs)?;

    let p384_voprf_tvs =
        json_to_test_vectors!(rfc, String::from("P384-SHA384"), String::from("VOPRF"));
    assert_ne!(p384_voprf_tvs.len(), 0);
    test_voprf_seed_to_key::<NistP384>(&p384_voprf_tvs)?;
    test_voprf_blind::<NistP384>(&p384_voprf_tvs)?;
    test_voprf_blind_evaluate::<NistP384>(&p384_voprf_tvs)?;
    test_voprf_finalize::<NistP384>(&p384_voprf_tvs)?;
    test_voprf_evaluate::<NistP384>(&p384_voprf_tvs)?;

    let p384_poprf_tvs =
        json_to_test_vectors!(rfc, String::from("P384-SHA384"), String::from("POPRF"));
    assert_ne!(p384_poprf_tvs.len(), 0);
    test_poprf_seed_to_key::<NistP384>(&p384_poprf_tvs)?;
    test_poprf_blind::<NistP384>(&p384_poprf_tvs)?;
    test_poprf_blind_evaluate::<NistP384>(&p384_poprf_tvs)?;
    test_poprf_finalize::<NistP384>(&p384_poprf_tvs)?;
    test_poprf_evaluate::<NistP384>(&p384_poprf_tvs)?;

    let p521_oprf_tvs =
        json_to_test_vectors!(rfc, String::from("P521-SHA512"), String::from("OPRF"));
    assert_ne!(p521_oprf_tvs.len(), 0);
    test_oprf_seed_to_key::<NistP521>(&p521_oprf_tvs)?;
    test_oprf_blind::<NistP521>(&p521_oprf_tvs)?;
    test_oprf_blind_evaluate::<NistP521>(&p521_oprf_tvs)?;
    test_oprf_finalize::<NistP521>(&p521_oprf_tvs)?;
    test_oprf_evaluate::<NistP521>(&p521_oprf_tvs)?;

    let p521_voprf_tvs =
        json_to_test_vectors!(rfc, String::from("P521-SHA512"), String::from("VOPRF"));
    assert_ne!(p521_voprf_tvs.len(), 0);
    test_voprf_seed_to_key::<NistP521>(&p521_voprf_tvs)?;
    test_voprf_blind::<NistP521>(&p521_voprf_tvs)?;
    test_voprf_blind_evaluate::<NistP521>(&p521_voprf_tvs)?;
    test_voprf_finalize::<NistP521>(&p521_voprf_tvs)?;
    test_voprf_evaluate::<NistP521>(&p521_voprf_tvs)?;

    let p521_poprf_tvs =
        json_to_test_vectors!(rfc, String::from("P521-SHA512"), String::from("POPRF"));
    assert_ne!(p521_poprf_tvs.len(), 0);
    test_poprf_seed_to_key::<NistP521>(&p521_poprf_tvs)?;
    test_poprf_blind::<NistP521>(&p521_poprf_tvs)?;
    test_poprf_blind_evaluate::<NistP521>(&p521_poprf_tvs)?;
    test_poprf_finalize::<NistP521>(&p521_poprf_tvs)?;
    test_poprf_evaluate::<NistP521>(&p521_poprf_tvs)?;

    Ok(())
}

fn test_oprf_seed_to_key<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        let server = OprfServer::<CS>::new_from_seed(&parameters.seed, &parameters.key_info)?;

        assert_eq!(
            &parameters.sksm,
            &CS::Group::serialize_scalar(server.get_private_key()).to_vec()
        );
    }
    Ok(())
}

fn test_voprf_seed_to_key<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        let server = VoprfServer::<CS>::new_from_seed(&parameters.seed, &parameters.key_info)?;

        assert_eq!(
            &parameters.sksm,
            &CS::Group::serialize_scalar(server.get_private_key()).to_vec()
        );
        assert_eq!(
            &parameters.pksm,
            CS::Group::serialize_elem(server.get_public_key()).as_slice()
        );
    }
    Ok(())
}

fn test_poprf_seed_to_key<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        let server = PoprfServer::<CS>::new_from_seed(&parameters.seed, &parameters.key_info)?;

        assert_eq!(
            &parameters.sksm,
            &CS::Group::serialize_scalar(server.get_private_key()).to_vec()
        );
        assert_eq!(
            &parameters.pksm,
            CS::Group::serialize_elem(server.get_public_key()).as_slice()
        );
    }
    Ok(())
}

// Tests input -> blind, blinded_element
fn test_oprf_blind<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        for i in 0..parameters.input.len() {
            let blind = CS::Group::deserialize_scalar(&parameters.blind[i])?;
            let client_result =
                OprfClient::<CS>::deterministic_blind_unchecked(&parameters.input[i], blind)?;

            assert_eq!(
                &parameters.blind[i],
                &CS::Group::serialize_scalar(client_result.state.blind).to_vec()
            );
            assert_eq!(
                parameters.blinded_element[i].as_slice(),
                client_result.message.serialize().as_slice(),
            );
        }
    }
    Ok(())
}

// Tests input -> blind, blinded_element
fn test_voprf_blind<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        for i in 0..parameters.input.len() {
            let blind = CS::Group::deserialize_scalar(&parameters.blind[i])?;
            let client_blind_result =
                VoprfClient::<CS>::deterministic_blind_unchecked(&parameters.input[i], blind)?;

            assert_eq!(
                &parameters.blind[i],
                &CS::Group::serialize_scalar(client_blind_result.state.get_blind()).to_vec()
            );
            assert_eq!(
                parameters.blinded_element[i].as_slice(),
                client_blind_result.message.serialize().as_slice(),
            );
        }
    }
    Ok(())
}

// Tests input -> blind, blinded_element
fn test_poprf_blind<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        for i in 0..parameters.input.len() {
            let blind = CS::Group::deserialize_scalar(&parameters.blind[i])?;
            let client_blind_result =
                PoprfClient::<CS>::deterministic_blind_unchecked(&parameters.input[i], blind)?;

            assert_eq!(
                &parameters.blind[i],
                &CS::Group::serialize_scalar(client_blind_result.state.get_blind()).to_vec()
            );
            assert_eq!(
                parameters.blinded_element[i].as_slice(),
                client_blind_result.message.serialize().as_slice(),
            );
        }
    }
    Ok(())
}

// Tests sksm, blinded_element -> evaluation_element
fn test_oprf_blind_evaluate<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        for i in 0..parameters.input.len() {
            let server = OprfServer::<CS>::new_with_key(&parameters.sksm)?;
            let message = server.blind_evaluate(&BlindedElement::deserialize(
                &parameters.blinded_element[i],
            )?);

            assert_eq!(
                &parameters.evaluation_element[i],
                &message.serialize().as_slice()
            );
        }
    }
    Ok(())
}

fn test_voprf_blind_evaluate<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ScalarLen>,
    Sum<<CS::Group as Group>::ScalarLen, <CS::Group as Group>::ScalarLen>: ArrayLength<u8>,
{
    for parameters in tvs {
        let mut rng = CycleRng::new(parameters.proof_random_scalar.clone());
        let server = VoprfServer::<CS>::new_with_key(&parameters.sksm)?;

        let mut blinded_elements = vec![];
        for blinded_element_bytes in &parameters.blinded_element {
            blinded_elements.push(BlindedElement::deserialize(blinded_element_bytes)?);
        }

        let prepared_evaluation_elements =
            server.batch_blind_evaluate_prepare(blinded_elements.iter());
        let prepared_elements: Vec<_> = prepared_evaluation_elements.collect();
        let VoprfServerBatchEvaluateFinishResult { messages, proof } = server
            .batch_blind_evaluate_finish(&mut rng, blinded_elements.iter(), &prepared_elements)?;
        let messages: Vec<_> = messages.collect();

        for (parameter, message) in parameters.evaluation_element.iter().zip(messages) {
            assert_eq!(&parameter, &message.serialize().as_slice());
        }

        assert_eq!(&parameters.proof, &proof.serialize().as_slice());
    }
    Ok(())
}

fn test_poprf_blind_evaluate<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ScalarLen>,
    Sum<<CS::Group as Group>::ScalarLen, <CS::Group as Group>::ScalarLen>: ArrayLength<u8>,
{
    for parameters in tvs {
        let mut rng = CycleRng::new(parameters.proof_random_scalar.clone());
        let server = PoprfServer::<CS>::new_with_key(&parameters.sksm)?;

        let mut blinded_elements = vec![];
        for blinded_element_bytes in &parameters.blinded_element {
            blinded_elements.push(BlindedElement::deserialize(blinded_element_bytes)?);
        }

        let PoprfServerBatchEvaluatePrepareResult {
            prepared_evaluation_elements,
            prepared_tweak,
        } = server.batch_blind_evaluate_prepare(blinded_elements.iter(), Some(&parameters.info))?;
        let prepared_evaluation_elements: Vec<_> = prepared_evaluation_elements.collect();
        let PoprfServerBatchEvaluateFinishResult { messages, proof } =
            PoprfServer::batch_blind_evaluate_finish::<_, _, Vec<_>>(
                &mut rng,
                blinded_elements.iter(),
                &prepared_evaluation_elements,
                &prepared_tweak,
            )
            .unwrap();

        let messages: Vec<_> = messages.collect();

        for (parameter, message) in parameters.evaluation_element.iter().zip(messages) {
            assert_eq!(&parameter, &message.serialize().as_slice());
        }

        assert_eq!(&parameters.proof, &proof.serialize().as_slice());
    }
    Ok(())
}

// Tests input, blind, evaluation_element -> output
fn test_oprf_finalize<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        for i in 0..parameters.input.len() {
            let client =
                OprfClient::<CS>::from_blind(CS::Group::deserialize_scalar(&parameters.blind[i])?);

            let client_finalize_result = client.finalize(
                &parameters.input[i],
                &EvaluationElement::deserialize(&parameters.evaluation_element[i])?,
            )?;

            assert_eq!(&parameters.output[i], &client_finalize_result.to_vec());
        }
    }
    Ok(())
}

fn test_voprf_finalize<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        let mut clients = vec![];
        for i in 0..parameters.input.len() {
            let client = VoprfClient::<CS>::from_blind_and_element(
                CS::Group::deserialize_scalar(&parameters.blind[i])?,
                CS::Group::deserialize_elem(&parameters.blinded_element[i])?,
            );
            clients.push(client.clone());
        }

        let messages: Vec<_> = parameters
            .evaluation_element
            .iter()
            .map(|x| EvaluationElement::deserialize(x).unwrap())
            .collect();

        let batch_result = VoprfClient::batch_finalize(
            &parameters.input,
            &clients,
            &messages,
            &Proof::deserialize(&parameters.proof)?,
            CS::Group::deserialize_elem(&parameters.pksm)?,
        )?;

        assert_eq!(
            parameters.output,
            batch_result
                .map(|arr| arr.map(|message| message.to_vec()))
                .collect::<Result<Vec<_>>>()?
        );
    }
    Ok(())
}

fn test_poprf_finalize<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        let mut clients = vec![];
        for i in 0..parameters.input.len() {
            let blind = CS::Group::deserialize_scalar(&parameters.blind[i])?;
            let client_blind_result =
                PoprfClient::<CS>::deterministic_blind_unchecked(&parameters.input[i], blind)?;
            let client = client_blind_result.state;
            clients.push(client.clone());
        }

        let messages: Vec<_> = parameters
            .evaluation_element
            .iter()
            .map(|x| EvaluationElement::deserialize(x).unwrap())
            .collect();

        let batch_result = PoprfClient::batch_finalize(
            parameters.input.iter().map(|input| input.as_slice()),
            &clients,
            &messages,
            &Proof::deserialize(&parameters.proof)?,
            CS::Group::deserialize_elem(&parameters.pksm)?,
            Some(&parameters.info),
        )?;

        let result: Vec<Vec<u8>> = batch_result.map(|arr| arr.unwrap().to_vec()).collect();

        assert_eq!(parameters.output, result);
    }
    Ok(())
}

// Tests input, sksm -> output
fn test_oprf_evaluate<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        for i in 0..parameters.input.len() {
            let server = OprfServer::<CS>::new_with_key(&parameters.sksm)?;

            let server_evaluate_result = server.evaluate(&parameters.input[i])?;

            assert_eq!(&parameters.output[i], &server_evaluate_result.to_vec());
        }
    }
    Ok(())
}

fn test_voprf_evaluate<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        for i in 0..parameters.input.len() {
            let server = VoprfServer::<CS>::new_with_key(&parameters.sksm)?;

            let server_evaluate_result = server.evaluate(&parameters.input[i])?;

            assert_eq!(&parameters.output[i], &server_evaluate_result.to_vec());
        }
    }
    Ok(())
}

fn test_poprf_evaluate<CS: CipherSuite>(tvs: &[VOPRFTestVectorParameters]) -> Result<()>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    for parameters in tvs {
        for i in 0..parameters.input.len() {
            let server = PoprfServer::<CS>::new_with_key(&parameters.sksm)?;

            let server_evaluate_result =
                server.evaluate(&parameters.input[i], Some(&parameters.info))?;

            assert_eq!(&parameters.output[i], &server_evaluate_result.to_vec());
        }
    }
    Ok(())
}
