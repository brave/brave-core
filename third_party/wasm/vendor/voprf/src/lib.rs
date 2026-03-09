// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! An implementation of a verifiable oblivious pseudorandom function (VOPRF)
//!
//! Note: This implementation is in sync with
//! [RFC 9497](https://www.rfc-editor.org/rfc/rfc9497).
//!
//! # Overview
//!
//! A verifiable oblivious pseudorandom function is a protocol that is evaluated
//! between a client and a server. They must first agree on a finite cyclic
//! group along with a point representation.
//!
//! We will use the following choice in this example:
//!
//! ```ignore
//! type CipherSuite = voprf::Ristretto255;
//! ```
//!
//! ## Modes of Operation
//!
//! VOPRF can be used in three modes:
//! - [Base Mode](#base-mode), which corresponds to a normal OPRF evaluation
//!   with no support for the verification of the OPRF outputs
//! - [Verifiable Mode](#verifiable-mode), which corresponds to an OPRF
//!   evaluation where the outputs can be verified against a server public key
//!   (VOPRF)
//! - [Partially Oblivious Verifiable Mode](#metadata), which corresponds to a
//!   VOPRF, where a public input can be supplied to the PRF computation
//!
//! In all of these modes, the protocol begins with a client blinding, followed
//! by a server evaluation, and finishes with a client finalization and server
//! evaluation.
//!
//! ## Base Mode
//!
//! In base mode, an [OprfClient] interacts with an [OprfServer] to compute the
//! output of the OPRF.
//!
//! ### Server Setup
//!
//! The protocol begins with a setup phase, in which the server must run
//! [OprfServer::new()] to produce an instance of itself. This instance must be
//! persisted on the server and used for online client evaluations.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! use rand::rngs::OsRng;
//! use rand::RngCore;
//! use voprf::OprfServer;
//!
//! let mut server_rng = OsRng;
//! let server = OprfServer::<CipherSuite>::new(&mut server_rng);
//! ```
//!
//! ### Client Blinding
//!
//! In the first step, the client chooses an input, and runs [OprfClient::blind]
//! to produce an [OprfClientBlindResult], which consists of a [BlindedElement]
//! to be sent to the server and an [OprfClient] which must be persisted on the
//! client for the final step of the VOPRF protocol.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! use rand::rngs::OsRng;
//! use rand::RngCore;
//! use voprf::OprfClient;
//!
//! let mut client_rng = OsRng;
//! let client_blind_result = OprfClient::<CipherSuite>::blind(b"input", &mut client_rng)
//!     .expect("Unable to construct client");
//! ```
//!
//! ### Server Blind Evaluation
//!
//! In the second step, the server takes as input the message from
//! [OprfClient::blind] (a [BlindedElement]), and runs
//! [OprfServer::blind_evaluate] to produce [EvaluationElement] to be sent to
//! the client.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::OprfClient;
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! # let mut client_rng = OsRng;
//! # let client_blind_result = OprfClient::<CipherSuite>::blind(
//! #     b"input",
//! #     &mut client_rng,
//! # ).expect("Unable to construct client");
//! # use voprf::OprfServer;
//! # let mut server_rng = OsRng;
//! # let server = OprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//! let server_evaluate_result = server.blind_evaluate(&client_blind_result.message);
//! ```
//!
//! ### Client Finalization
//!
//! In the final step on the client side, the client takes as input the message
//! from [OprfServer::evaluate] (an [EvaluationElement]), and runs
//! [OprfClient::finalize] to produce an output for the protocol.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::OprfClient;
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! # let mut client_rng = OsRng;
//! # let client_blind_result = OprfClient::<CipherSuite>::blind(
//! #     b"input",
//! #     &mut client_rng,
//! # ).expect("Unable to construct client");
//! # use voprf::OprfServer;
//! # let mut server_rng = OsRng;
//! # let server = OprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//! # let message = server.blind_evaluate(&client_blind_result.message);
//! let client_finalize_result = client_blind_result
//!     .state
//!     .finalize(b"input", &message)
//!     .expect("Unable to perform client finalization");
//!
//! println!("VOPRF output: {:?}", client_finalize_result.to_vec());
//! ```
//!
//! ### Server Evaluation
//!
//! Optionally, if the server has direct access to the PRF input, then it need
//! not perform the oblivious computation and can simply run
//! [OprfServer::evaluate] to generate an output which matches the output
//! produced by an execution of the oblivious protocol on the same input and
//! key.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::OprfClient;
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! # let mut client_rng = OsRng;
//! # let client_blind_result = OprfClient::<CipherSuite>::blind(
//! #     b"input",
//! #     &mut client_rng,
//! # ).expect("Unable to construct client");
//! # use voprf::OprfServer;
//! # let mut server_rng = OsRng;
//! # let server = OprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//! # let message = server.blind_evaluate(&client_blind_result.message);
//! let client_finalize_result = client_blind_result
//!     .state
//!     .finalize(b"input", &message)
//!     .expect("Unable to perform client finalization");
//!
//! let server_evaluate_result = server
//!     .evaluate(b"input")
//!     .expect("Unable to perform the server evaluation");
//!
//! assert_eq!(client_finalize_result, server_evaluate_result);
//! ```
//!
//! ## Verifiable Mode
//!
//! In verifiable mode, a [VoprfClient] interacts with a [VoprfServer] to
//! compute the output of the VOPRF. In order to verify the server's
//! computation, the client checks a server-generated proof against the server's
//! public key. If the proof fails to verify, then the client does not receive
//! an output.
//!
//! In batch mode, a single proof can be used for multiple VOPRF evaluations.
//! See [the batching section](#batching) for more details on how to perform
//! batch evaluations.
//!
//! ### Server Setup
//!
//! The protocol begins with a setup phase, in which the server must run
//! [VoprfServer::new()] to produce an instance of itself. This instance must be
//! persisted on the server and used for online client evaluations.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! use rand::rngs::OsRng;
//! use rand::RngCore;
//! use voprf::VoprfServer;
//!
//! let mut server_rng = OsRng;
//! let server = VoprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//!
//! // To be sent to the client
//! println!("Server public key: {:?}", server.get_public_key());
//! ```
//!
//! The public key should be sent to the client, since the client will need it
//! in the final step of the protocol in order to complete the evaluation of the
//! VOPRF.
//!
//! ### Client Blinding
//!
//! In the first step, the client chooses an input, and runs
//! [VoprfClient::blind] to produce a [VoprfClientBlindResult], which consists
//! of a [BlindedElement] to be sent to the server and a [VoprfClient] which
//! must be persisted on the client for the final step of the VOPRF protocol.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! use rand::rngs::OsRng;
//! use rand::RngCore;
//! use voprf::VoprfClient;
//!
//! let mut client_rng = OsRng;
//! let client_blind_result = VoprfClient::<CipherSuite>::blind(b"input", &mut client_rng)
//!     .expect("Unable to construct client");
//! ```
//!
//! ### Server Blind Evaluation
//!
//! In the second step, the server takes as input the message from
//! [VoprfClient::blind] (a [BlindedElement]), and runs
//! [VoprfServer::blind_evaluate] to produce a [VoprfServerEvaluateResult],
//! which consists of an [EvaluationElement] to be sent to the client along with
//! a proof.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::{VoprfServerEvaluateResult, VoprfClient};
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! # let mut client_rng = OsRng;
//! # let client_blind_result = VoprfClient::<CipherSuite>::blind(
//! #     b"input",
//! #     &mut client_rng,
//! # ).expect("Unable to construct client");
//! # use voprf::VoprfServer;
//! # let mut server_rng = OsRng;
//! # let server = VoprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//! let VoprfServerEvaluateResult { message, proof } =
//!     server.blind_evaluate(&mut server_rng, &client_blind_result.message);
//! ```
//!
//! ### Client Finalization
//!
//! In the final step, the client takes as input the message from
//! [VoprfServer::blind_evaluate] (an [EvaluationElement]), the proof, and the
//! server's public key, and runs [VoprfClient::finalize] to produce an output
//! for the protocol.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::VoprfClient;
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! # let mut client_rng = OsRng;
//! # let client_blind_result = VoprfClient::<CipherSuite>::blind(
//! #     b"input",
//! #     &mut client_rng,
//! # ).expect("Unable to construct client");
//! # use voprf::VoprfServer;
//! # let mut server_rng = OsRng;
//! # let server = VoprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//! # let server_evaluate_result = server.blind_evaluate(
//! #     &mut server_rng,
//! #     &client_blind_result.message,
//! # );
//! let client_finalize_result = client_blind_result
//!     .state
//!     .finalize(
//!         b"input",
//!         &server_evaluate_result.message,
//!         &server_evaluate_result.proof,
//!         server.get_public_key(),
//!     )
//!     .expect("Unable to perform client finalization");
//!
//! println!("VOPRF output: {:?}", client_finalize_result.to_vec());
//! ```
//!
//! ### Server Evaluation
//!
//! Optionally, if the server has direct access to the PRF input, then it need
//! not perform the oblivious computation and can simply run
//! [VoprfServer::evaluate] to generate an output which matches the output
//! produced by an execution of the oblivious protocol on the same input and
//! key.
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::VoprfClient;
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! # let mut client_rng = OsRng;
//! # let client_blind_result = VoprfClient::<CipherSuite>::blind(
//! #     b"input",
//! #     &mut client_rng,
//! # ).expect("Unable to construct client");
//! # use voprf::VoprfServer;
//! # let mut server_rng = OsRng;
//! # let server = VoprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//! # let server_evaluate_result = server.blind_evaluate(
//! #     &mut server_rng,
//! #     &client_blind_result.message,
//! # );
//! let client_finalize_result = client_blind_result
//!     .state
//!     .finalize(
//!         b"input",
//!         &server_evaluate_result.message,
//!         &server_evaluate_result.proof,
//!         server.get_public_key(),
//!     )
//!     .expect("Unable to perform client finalization");
//!
//! let server_evaluate_result = server
//!     .evaluate(b"input")
//!     .expect("Unable to perform the server evaluation");
//!
//! assert_eq!(client_finalize_result, server_evaluate_result);
//! ```
//!
//! # Advanced Usage
//!
//! There are two additional (and optional) extensions to the core VOPRF
//! protocol: support for batching of evaluations, and support for public
//! metadata.
//!
//! ## Batching
//!
//! It is sometimes desirable to generate only a single, constant-size proof for
//! an unbounded number of VOPRF evaluations (on arbitrary inputs).
//! [VoprfClient] and [VoprfServer] support a batch API for handling this case.
//! In the following example, we show how to use the batch API to produce a
//! single proof for 10 parallel VOPRF evaluations.
//!
//! First, the client produces 10 blindings, storing their resulting states and
//! messages:
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::VoprfClient;
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! let mut client_rng = OsRng;
//! let mut client_states = vec![];
//! let mut client_messages = vec![];
//! for _ in 0..10 {
//!     let client_blind_result = VoprfClient::<CipherSuite>::blind(b"input", &mut client_rng)
//!         .expect("Unable to construct client");
//!     client_states.push(client_blind_result.state);
//!     client_messages.push(client_blind_result.message);
//! }
//! ```
//!
//! Next, the server calls the [VoprfServer::batch_blind_evaluate_prepare] and
//! [VoprfServer::batch_blind_evaluate_finish] function on a set of client
//! messages, to produce a corresponding set of messages to be returned to the
//! client (returned in the same order), along with a single proof:
//!
//! ```
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::{VoprfServerBatchEvaluateFinishResult, VoprfClient};
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! # let mut client_rng = OsRng;
//! # let mut client_states = vec![];
//! # let mut client_messages = vec![];
//! # for _ in 0..10 {
//! #     let client_blind_result = VoprfClient::<CipherSuite>::blind(
//! #         b"input",
//! #        &mut client_rng,
//! #     ).expect("Unable to construct client");
//! #     client_states.push(client_blind_result.state);
//! #     client_messages.push(client_blind_result.message);
//! # }
//! # use voprf::VoprfServer;
//! let mut server_rng = OsRng;
//! # let server = VoprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//! let prepared_evaluation_elements = server.batch_blind_evaluate_prepare(client_messages.iter());
//! let prepared_elements: Vec<_> = prepared_evaluation_elements.collect();
//! let VoprfServerBatchEvaluateFinishResult { messages, proof } = server
//!     .batch_blind_evaluate_finish(&mut server_rng, client_messages.iter(), &prepared_elements)
//!     .expect("Unable to perform server batch evaluate");
//! let messages: Vec<_> = messages.collect();
//! ```
//!
//! If `alloc` is available, `VoprfServer::batch_blind_evaluate` can be called
//! to avoid having to collect output manually:
//!
//! ```
//! # #[cfg(feature = "alloc")] {
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::{VoprfServerBatchEvaluateResult, VoprfClient};
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! # let mut client_rng = OsRng;
//! # let mut client_states = vec![];
//! # let mut client_messages = vec![];
//! # for _ in 0..10 {
//! #     let client_blind_result = VoprfClient::<CipherSuite>::blind(
//! #         b"input",
//! #        &mut client_rng,
//! #     ).expect("Unable to construct client");
//! #     client_states.push(client_blind_result.state);
//! #     client_messages.push(client_blind_result.message);
//! # }
//! # use voprf::VoprfServer;
//! let mut server_rng = OsRng;
//! # let server = VoprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//! let VoprfServerBatchEvaluateResult { messages, proof } = server
//!     .batch_blind_evaluate(&mut server_rng, &client_messages)
//!     .expect("Unable to perform server batch evaluate");
//! # }
//! ```
//!
//! Then, the client calls [VoprfClient::batch_finalize] on the client states
//! saved from the first step, along with the messages returned by the server,
//! along with the server's proof, in order to produce a vector of outputs if
//! the proof verifies correctly.
//!
//! ```
//! # #[cfg(feature = "alloc")] {
//! # #[cfg(feature = "ristretto255")]
//! # type CipherSuite = voprf::Ristretto255;
//! # #[cfg(not(feature = "ristretto255"))]
//! # type CipherSuite = p256::NistP256;
//! # use voprf::{VoprfServerBatchEvaluateResult, VoprfClient};
//! # use rand::{rngs::OsRng, RngCore};
//! #
//! # let mut client_rng = OsRng;
//! # let mut client_states = vec![];
//! # let mut client_messages = vec![];
//! # for _ in 0..10 {
//! #     let client_blind_result = VoprfClient::<CipherSuite>::blind(
//! #         b"input",
//! #        &mut client_rng,
//! #     ).expect("Unable to construct client");
//! #     client_states.push(client_blind_result.state);
//! #     client_messages.push(client_blind_result.message);
//! # }
//! # use voprf::VoprfServer;
//! # let mut server_rng = OsRng;
//! # let server = VoprfServer::<CipherSuite>::new(&mut server_rng).unwrap();
//! # let VoprfServerBatchEvaluateResult { messages, proof } = server
//! #     .batch_blind_evaluate(&mut server_rng, &client_messages)
//! #     .expect("Unable to perform server batch evaluate");
//! let client_batch_finalize_result = VoprfClient::batch_finalize(
//!     &[b"input"; 10],
//!     &client_states,
//!     &messages,
//!     &proof,
//!     server.get_public_key(),
//! )
//! .expect("Unable to perform client batch finalization")
//! .collect::<Vec<_>>();
//!
//! println!("VOPRF batch outputs: {:?}", client_batch_finalize_result);
//! # }
//! ```
//!
//! ## Metadata
//!
//! The optional metadata parameter included in the POPRF mode allows clients
//! and servers to cryptographically bind additional data to the VOPRF output.
//! This metadata is known to both parties at the start of the protocol, and is
//! inserted under the server's blind evaluate step and the client's finalize
//! step. This metadata can be constructed with some type of higher-level domain
//! separation to avoid cross-protocol attacks or related issues.
//!
//! The API for POPRF mode is similar to VOPRF mode, except that a [PoprfServer]
//! and [PoprfClient] are used, and that each of the functions accept an
//! additional (and optional) info parameter which represents the public input.
//! See
//! <https://www.rfc-editor.org/rfc/rfc9497#name-poprf-public-input>
//! for more detailed information on how this public input should be used.
//!
//! # Features
//!
//! - The `alloc` feature requires Rust's `alloc` crate and enables batching
//!   VOPRF evaluations.
//!
//! - The `serde` feature, enabled by default, provides convenience functions
//!   for serializing and deserializing with [serde](https://serde.rs/).
//!
//! - The `danger` feature, disabled by default, exposes functions for setting
//!   and getting internal values not available in the default API. These
//!   functions are intended for use in by higher-level cryptographic protocols
//!   that need access to these raw values and are able to perform the necessary
//!   validations on them (such as being valid group elements).
//!
//! - The `ristretto255-ciphersuite` features enables using [`Ristretto255`] as
//!   a [`CipherSuite`].
//!
//! - The `ristretto255` feature enables using [`Ristretto255`] as the
//!   underlying group for the [Group] choice. To select a specific backend see
//!   the [curve25519-dalek] documentation.
//!
//! [curve25519-dalek]:
//!     (https://docs.rs/curve25519-dalek/4.0.0-pre.5/curve25519_dalek/index.html#backends)

#![no_std]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]
#![cfg_attr(not(test), deny(unsafe_code))]
#![warn(
    clippy::cargo,
    clippy::missing_errors_doc,
    missing_debug_implementations,
    missing_docs
)]
#![allow(clippy::multiple_crate_versions)]

#[cfg(any(feature = "alloc", test))]
extern crate alloc;

#[cfg(feature = "std")]
extern crate std;

mod ciphersuite;
mod common;
mod error;
mod group;
mod oprf;
mod poprf;
mod serialization;
mod voprf;

#[cfg(test)]
mod tests;

// Exports

pub use crate::ciphersuite::CipherSuite;
#[cfg(feature = "danger")]
pub use crate::common::derive_key;
pub use crate::common::{
    BlindedElement, EvaluationElement, Mode, PreparedEvaluationElement, Proof,
};
pub use crate::error::{Error, InternalError, Result};
pub use crate::group::Group;
#[cfg(feature = "ristretto255")]
pub use crate::group::Ristretto255;
pub use crate::oprf::{OprfClient, OprfClientBlindResult, OprfServer};
#[cfg(feature = "alloc")]
pub use crate::poprf::PoprfServerBatchEvaluateResult;
pub use crate::poprf::{
    PoprfClient, PoprfClientBatchFinalizeResult, PoprfPreparedTweak, PoprfServer,
    PoprfServerBatchEvaluateFinishResult, PoprfServerBatchEvaluateFinishedMessages,
    PoprfServerBatchEvaluatePrepareResult, PoprfServerBatchEvaluatePreparedEvaluationElements,
};
pub use crate::serialization::{
    BlindedElementLen, EvaluationElementLen, OprfClientLen, OprfServerLen, PoprfClientLen,
    PoprfServerLen, ProofLen, VoprfClientLen, VoprfServerLen,
};
#[cfg(feature = "alloc")]
pub use crate::voprf::VoprfServerBatchEvaluateResult;
pub use crate::voprf::{
    VoprfClient, VoprfClientBatchFinalizeResult, VoprfClientBlindResult, VoprfServer,
    VoprfServerBatchEvaluateFinishResult, VoprfServerBatchEvaluateFinishedMessages,
    VoprfServerBatchEvaluatePreparedEvaluationElements, VoprfServerEvaluateResult,
};
