// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use core::str::Utf8Error;

use challenge_bypass_ristretto::{errors, voprf};
use cxx::{CxxString, CxxVector};
use derive_more::From;
use hmac::Hmac;
use lazy_static::lazy_static;
use rand::rngs::OsRng;
use sha2::Sha512;

type HmacSha512 = Hmac<Sha512>;

lazy_static! {
    static ref NO_ERROR: Error = Error::none();
}

macro_rules! impl_result {
    ($t:ident, $r:ident) => {
        impl $r {
            fn error(self: &$r) -> &Error {
                match &self.0 {
                    Err(e) => &e,
                    Ok(_) => &NO_ERROR,
                }
            }

            fn is_ok(self: &$r) -> bool {
                self.error().is_ok()
            }

            fn unwrap(self: &$r) -> &$t {
                self.0.as_ref().expect(
                    "Unhandled error before unwrap call, self->error().code != TokenError::None",
                )
            }
        }
    };
}

macro_rules! impl_decode_base64 {
    ($f:ident, $t:ident, $r:ident) => {
        fn $f(s: &str) -> Box<$r> {
            Box::new(|| -> Result<$t, Error> { Ok(voprf::$t::decode_base64(&s)?.into()) }().into())
        }
    };
}

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = cbr_cxx)]
pub mod ffi {
    #[derive(Debug)]
    struct Error {
        code: TokenError,
        msg: String,
    }

    #[derive(Debug)]
    enum TokenError {
        // No error occured
        None,
        // An error occured when converting from a `CompressedRistretto` to a `RistrettoPoint`
        PointDecompressionError,
        // An error occured when interpretting bytes as a scalar
        ScalarFormatError,
        // An error in the length of bytes handed to a constructor.
        BytesLengthError,
        // Verification failed
        VerifyError,
        // Inputs differed in length
        LengthMismatchError,
        // Decoding failed
        DecodingError,
    }

    extern "Rust" {
        type TokenPreimage;
        type Token;
        type BlindedToken;
        type SignedToken;
        type UnblindedToken;
        type SigningKey;
        type PublicKey;
        type VerificationKey;
        type VerificationSignature;
        type BatchDLEQProof;
        type Tokens;
        type BlindedTokens;
        type SignedTokens;
        type UnblindedTokens;

        type TokenPreimageResult;
        type TokenResult;
        type BlindedTokenResult;
        type SignedTokenResult;
        type UnblindedTokenResult;
        type SigningKeyResult;
        type PublicKeyResult;
        type VerificationSignatureResult;
        type BatchDLEQProofResult;
        type TokensResult;
        type BlindedTokensResult;
        type SignedTokensResult;
        type UnblindedTokensResult;

        fn is_ok(self: &Error) -> bool;

        fn encode_base64(self: &TokenPreimage) -> String;

        fn decode_base64_token_preimage(s: &str) -> Box<TokenPreimageResult>;
        fn error(self: &TokenPreimageResult) -> &Error;
        fn is_ok(self: &TokenPreimageResult) -> bool;
        fn unwrap(self: &TokenPreimageResult) -> &TokenPreimage;

        fn generate_token() -> Box<Token>;
        fn encode_base64(self: &Token) -> String;
        fn blind(self: &Token) -> Box<BlindedToken>;

        fn decode_base64_token(s: &str) -> Box<TokenResult>;
        fn error(self: &TokenResult) -> &Error;
        fn is_ok(self: &TokenResult) -> bool;
        fn unwrap(self: &TokenResult) -> &Token;

        fn encode_base64(self: &BlindedToken) -> String;

        fn decode_base64_blinded_token(s: &str) -> Box<BlindedTokenResult>;
        fn error(self: &BlindedTokenResult) -> &Error;
        fn is_ok(self: &BlindedTokenResult) -> bool;
        fn unwrap(self: &BlindedTokenResult) -> &BlindedToken;

        fn encode_base64(self: &SignedToken) -> String;

        fn decode_base64_signed_token(s: &str) -> Box<SignedTokenResult>;
        fn error(self: &SignedTokenResult) -> &Error;
        fn is_ok(self: &SignedTokenResult) -> bool;
        fn unwrap(self: &SignedTokenResult) -> &SignedToken;

        fn encode_base64(self: &UnblindedToken) -> String;
        fn derive_verification_key(self: &UnblindedToken) -> Box<VerificationKey>;
        fn preimage(self: &UnblindedToken) -> Box<TokenPreimage>;

        fn decode_base64_unblinded_token(s: &str) -> Box<UnblindedTokenResult>;
        fn error(self: &UnblindedTokenResult) -> &Error;
        fn is_ok(self: &UnblindedTokenResult) -> bool;
        fn unwrap(self: &UnblindedTokenResult) -> &UnblindedToken;

        fn generate_signing_key() -> Box<SigningKey>;
        fn encode_base64(self: &SigningKey) -> String;
        fn public_key(self: &SigningKey) -> Box<PublicKey>;
        fn sign(self: &SigningKey, token: &BlindedToken) -> Box<SignedTokenResult>;
        fn rederive_unblinded_token(self: &SigningKey, t: &TokenPreimage) -> Box<UnblindedToken>;

        fn decode_base64_signing_key(s: &str) -> Box<SigningKeyResult>;
        fn error(self: &SigningKeyResult) -> &Error;
        fn is_ok(self: &SigningKeyResult) -> bool;
        fn unwrap(self: &SigningKeyResult) -> &SigningKey;

        fn encode_base64(self: &PublicKey) -> String;

        fn decode_base64_public_key(s: &str) -> Box<PublicKeyResult>;
        fn error(self: &PublicKeyResult) -> &Error;
        fn is_ok(self: &PublicKeyResult) -> bool;
        fn unwrap(self: &PublicKeyResult) -> &PublicKey;

        fn sign(self: &VerificationKey, msg: &CxxString) -> Box<VerificationSignature>;
        fn verify(self: &VerificationKey, sig: &VerificationSignature, msg: &CxxString) -> bool;

        fn encode_base64(self: &VerificationSignature) -> String;

        fn decode_base64_verification_signature(s: &str) -> Box<VerificationSignatureResult>;
        fn error(self: &VerificationSignatureResult) -> &Error;
        fn is_ok(self: &VerificationSignatureResult) -> bool;
        fn unwrap(self: &VerificationSignatureResult) -> &VerificationSignature;

        fn new_batch_dleq_proof(
            self: &SigningKey,
            blinded_tokens: &BlindedTokens,
            signed_tokens: &SignedTokens,
        ) -> Box<BatchDLEQProofResult>;

        fn encode_base64(self: &BatchDLEQProof) -> String;
        fn verify(
            self: &BatchDLEQProof,
            blinded_tokens: &BlindedTokens,
            signed_tokens: &SignedTokens,
            public_key: &PublicKey,
        ) -> Error;
        fn verify_and_unblind(
            self: &BatchDLEQProof,
            tokens: &Tokens,
            blinded_tokens: &BlindedTokens,
            signed_tokens: &SignedTokens,
            public_key: &PublicKey,
        ) -> Box<UnblindedTokensResult>;

        fn decode_base64_batch_dleq_proof(s: &str) -> Box<BatchDLEQProofResult>;
        fn error(self: &BatchDLEQProofResult) -> &Error;
        fn is_ok(self: &BatchDLEQProofResult) -> bool;
        fn unwrap(self: &BatchDLEQProofResult) -> &BatchDLEQProof;

        fn decode_base64_tokens(s: &CxxVector<CxxString>) -> Box<TokensResult>;
        fn error(self: &TokensResult) -> &Error;
        fn is_ok(self: &TokensResult) -> bool;
        fn unwrap(self: &TokensResult) -> &Tokens;

        fn decode_base64_blinded_tokens(s: &CxxVector<CxxString>) -> Box<BlindedTokensResult>;
        fn error(self: &BlindedTokensResult) -> &Error;
        fn is_ok(self: &BlindedTokensResult) -> bool;
        fn unwrap(self: &BlindedTokensResult) -> &BlindedTokens;

        fn decode_base64_signed_tokens(s: &CxxVector<CxxString>) -> Box<SignedTokensResult>;
        fn error(self: &SignedTokensResult) -> &Error;
        fn is_ok(self: &SignedTokensResult) -> bool;
        fn unwrap(self: &SignedTokensResult) -> &SignedTokens;

        fn decode_base64_unblinded_tokens(s: &CxxVector<CxxString>) -> Box<UnblindedTokensResult>;
        fn error(self: &UnblindedTokensResult) -> &Error;
        fn is_ok(self: &UnblindedTokensResult) -> bool;
        fn unwrap(self: &UnblindedTokensResult) -> &UnblindedTokens;

        fn as_vec(self: &UnblindedTokens) -> &Vec<UnblindedToken>;
    }
}

use ffi::*;

#[derive(From)]
pub struct TokenPreimage(voprf::TokenPreimage);
#[derive(From)]
pub struct Token(voprf::Token);
#[derive(From)]
pub struct BlindedToken(voprf::BlindedToken);
#[derive(From)]
pub struct SignedToken(voprf::SignedToken);
#[derive(From)]
pub struct UnblindedToken(voprf::UnblindedToken);
#[derive(From)]
pub struct SigningKey(voprf::SigningKey);
#[derive(From)]
pub struct PublicKey(voprf::PublicKey);
#[derive(From)]
pub struct VerificationKey(voprf::VerificationKey);
#[derive(From)]
pub struct VerificationSignature(voprf::VerificationSignature);
#[derive(From)]
pub struct BatchDLEQProof(voprf::BatchDLEQProof);
#[derive(From)]
pub struct Tokens(Vec<voprf::Token>);
#[derive(From)]
pub struct BlindedTokens(Vec<voprf::BlindedToken>);
#[derive(From)]
pub struct SignedTokens(Vec<voprf::SignedToken>);
#[derive(From)]
pub struct UnblindedTokens(Vec<UnblindedToken>);

impl Error {
    fn is_ok(self: &Error) -> bool {
        self.code == TokenError::None
    }

    fn none() -> Self {
        Error { code: TokenError::None, msg: "".to_string() }
    }
}

impl From<TokenError> for Error {
    fn from(error: TokenError) -> Self {
        Error { msg: "fixme".to_string(), code: error.into() }
    }
}

impl From<errors::TokenError> for Error {
    fn from(error: errors::TokenError) -> Self {
        Error { msg: error.to_string(), code: error.into() }
    }
}

impl From<errors::TokenError> for TokenError {
    fn from(error: errors::TokenError) -> Self {
        match error.0 {
            errors::InternalError::PointDecompressionError => TokenError::PointDecompressionError,
            errors::InternalError::ScalarFormatError => TokenError::ScalarFormatError,
            errors::InternalError::BytesLengthError { name: _, length: _ } => {
                TokenError::BytesLengthError
            }
            errors::InternalError::VerifyError => TokenError::VerifyError,
            errors::InternalError::LengthMismatchError => TokenError::LengthMismatchError,
            errors::InternalError::DecodingError => TokenError::DecodingError,
        }
    }
}

impl From<Utf8Error> for TokenError {
    fn from(_error: Utf8Error) -> Self {
        TokenError::DecodingError
    }
}

impl TokenPreimage {
    fn encode_base64(self: &TokenPreimage) -> String {
        self.0.encode_base64()
    }
}

#[derive(From)]
struct TokenPreimageResult(Result<TokenPreimage, Error>);

impl_decode_base64!(decode_base64_token_preimage, TokenPreimage, TokenPreimageResult);
impl_result!(TokenPreimage, TokenPreimageResult);

fn generate_token() -> Box<Token> {
    let mut rng = OsRng;
    Box::new(voprf::Token::random::<Sha512, OsRng>(&mut rng).into())
}

impl Token {
    fn encode_base64(self: &Token) -> String {
        self.0.encode_base64()
    }

    fn blind(self: &Token) -> Box<BlindedToken> {
        Box::new(self.0.blind().into())
    }
}

#[derive(From)]
struct TokenResult(Result<Token, Error>);

impl_decode_base64!(decode_base64_token, Token, TokenResult);
impl_result!(Token, TokenResult);

impl BlindedToken {
    fn encode_base64(self: &BlindedToken) -> String {
        self.0.encode_base64()
    }
}

#[derive(From)]
struct BlindedTokenResult(Result<BlindedToken, Error>);

impl_decode_base64!(decode_base64_blinded_token, BlindedToken, BlindedTokenResult);
impl_result!(BlindedToken, BlindedTokenResult);

impl SignedToken {
    fn encode_base64(self: &SignedToken) -> String {
        self.0.encode_base64()
    }
}

#[derive(From)]
struct SignedTokenResult(Result<SignedToken, Error>);

impl_decode_base64!(decode_base64_signed_token, SignedToken, SignedTokenResult);
impl_result!(SignedToken, SignedTokenResult);

impl UnblindedToken {
    fn encode_base64(self: &UnblindedToken) -> String {
        self.0.encode_base64()
    }

    fn derive_verification_key(self: &UnblindedToken) -> Box<VerificationKey> {
        Box::new(self.0.derive_verification_key::<Sha512>().into())
    }

    fn preimage(self: &UnblindedToken) -> Box<TokenPreimage> {
        Box::new(self.0.t.into())
    }
}

#[derive(From)]
struct UnblindedTokenResult(Result<UnblindedToken, Error>);

impl_decode_base64!(decode_base64_unblinded_token, UnblindedToken, UnblindedTokenResult);
impl_result!(UnblindedToken, UnblindedTokenResult);

fn generate_signing_key() -> Box<SigningKey> {
    let mut rng = OsRng;
    Box::new(voprf::SigningKey::random::<OsRng>(&mut rng).into())
}

impl SigningKey {
    fn encode_base64(self: &SigningKey) -> String {
        self.0.encode_base64()
    }

    fn public_key(self: &SigningKey) -> Box<PublicKey> {
        Box::new(self.0.public_key.into())
    }

    fn sign(self: &SigningKey, token: &BlindedToken) -> Box<SignedTokenResult> {
        Box::new(|| -> Result<SignedToken, Error> { Ok(self.0.sign(&token.0)?.into()) }().into())
    }

    fn rederive_unblinded_token(self: &SigningKey, t: &TokenPreimage) -> Box<UnblindedToken> {
        Box::new(self.0.rederive_unblinded_token(&t.0).into())
    }

    fn new_batch_dleq_proof(
        self: &SigningKey,
        blinded_tokens: &BlindedTokens,
        signed_tokens: &SignedTokens,
    ) -> Box<BatchDLEQProofResult> {
        let mut rng = OsRng;
        Box::new(
            || -> Result<BatchDLEQProof, Error> {
                Ok(voprf::BatchDLEQProof::new::<Sha512, OsRng>(
                    &mut rng,
                    &blinded_tokens.0,
                    &signed_tokens.0,
                    &self.0,
                )?
                .into())
            }()
            .map_err(|e| e.into())
            .into(),
        )
    }
}

#[derive(From)]
struct SigningKeyResult(Result<SigningKey, Error>);

impl_decode_base64!(decode_base64_signing_key, SigningKey, SigningKeyResult);
impl_result!(SigningKey, SigningKeyResult);

impl PublicKey {
    fn encode_base64(self: &PublicKey) -> String {
        self.0.encode_base64()
    }
}

#[derive(From)]
struct PublicKeyResult(Result<PublicKey, Error>);

impl_decode_base64!(decode_base64_public_key, PublicKey, PublicKeyResult);
impl_result!(PublicKey, PublicKeyResult);

impl VerificationKey {
    fn sign(self: &VerificationKey, msg: &CxxString) -> Box<VerificationSignature> {
        Box::new(self.0.sign::<HmacSha512>(msg.as_bytes()).into())
    }

    fn verify(self: &VerificationKey, sig: &VerificationSignature, msg: &CxxString) -> bool {
        self.0.verify::<HmacSha512>(&sig.0, msg.as_bytes())
    }
}

impl VerificationSignature {
    fn encode_base64(self: &VerificationSignature) -> String {
        self.0.encode_base64()
    }
}

#[derive(From)]
struct VerificationSignatureResult(Result<VerificationSignature, Error>);

impl_decode_base64!(
    decode_base64_verification_signature,
    VerificationSignature,
    VerificationSignatureResult
);
impl_result!(VerificationSignature, VerificationSignatureResult);

#[derive(From)]
struct TokensResult(Result<Tokens, Error>);

fn decode_base64_tokens(strings: &CxxVector<CxxString>) -> Box<TokensResult> {
    Box::new(
        || -> Result<Tokens, TokenError> {
            let mut tokens: Vec<voprf::Token> = Vec::new();
            for s in strings {
                let token = voprf::Token::decode_base64(s.to_str()?)?;
                tokens.push(token);
            }
            Ok(tokens.into())
        }()
        .map_err(|e| e.into())
        .into(),
    )
}
impl_result!(Tokens, TokensResult);

#[derive(From)]
struct BlindedTokensResult(Result<BlindedTokens, Error>);

fn decode_base64_blinded_tokens(strings: &CxxVector<CxxString>) -> Box<BlindedTokensResult> {
    Box::new(
        || -> Result<BlindedTokens, TokenError> {
            let mut tokens: Vec<voprf::BlindedToken> = Vec::new();
            for s in strings {
                let token = voprf::BlindedToken::decode_base64(s.to_str()?)?;
                tokens.push(token);
            }
            Ok(tokens.into())
        }()
        .map_err(|e| e.into())
        .into(),
    )
}
impl_result!(BlindedTokens, BlindedTokensResult);

#[derive(From)]
struct SignedTokensResult(Result<SignedTokens, Error>);

fn decode_base64_signed_tokens(strings: &CxxVector<CxxString>) -> Box<SignedTokensResult> {
    Box::new(
        || -> Result<SignedTokens, TokenError> {
            let mut tokens: Vec<voprf::SignedToken> = Vec::new();
            for s in strings {
                let token = voprf::SignedToken::decode_base64(s.to_str()?)?;
                tokens.push(token);
            }
            Ok(tokens.into())
        }()
        .map_err(|e| e.into())
        .into(),
    )
}
impl_result!(SignedTokens, SignedTokensResult);

impl UnblindedTokens {
    fn as_vec(self: &UnblindedTokens) -> &Vec<UnblindedToken> {
        &self.0
    }
}

#[derive(From)]
struct UnblindedTokensResult(Result<UnblindedTokens, Error>);

fn decode_base64_unblinded_tokens(strings: &CxxVector<CxxString>) -> Box<UnblindedTokensResult> {
    Box::new(
        || -> Result<UnblindedTokens, TokenError> {
            let mut tokens: Vec<UnblindedToken> = Vec::new();
            for s in strings {
                let token = voprf::UnblindedToken::decode_base64(s.to_str()?)?;
                tokens.push(token.into());
            }
            Ok(tokens.into())
        }()
        .map_err(|e| e.into())
        .into(),
    )
}
impl_result!(UnblindedTokens, UnblindedTokensResult);

impl BatchDLEQProof {
    fn encode_base64(self: &BatchDLEQProof) -> String {
        self.0.encode_base64()
    }

    fn verify(
        self: &BatchDLEQProof,
        blinded_tokens: &BlindedTokens,
        signed_tokens: &SignedTokens,
        public_key: &PublicKey,
    ) -> Error {
        match self.0.verify::<Sha512>(&blinded_tokens.0, &signed_tokens.0, &public_key.0) {
            Ok(_) => Error::none(),
            Err(e) => e.into(),
        }
    }

    fn verify_and_unblind(
        self: &BatchDLEQProof,
        tokens: &Tokens,
        blinded_tokens: &BlindedTokens,
        signed_tokens: &SignedTokens,
        public_key: &PublicKey,
    ) -> Box<UnblindedTokensResult> {
        Box::new(
            || -> Result<UnblindedTokens, Error> {
                Ok(self
                    .0
                    .verify_and_unblind::<Sha512, _>(
                        &tokens.0,
                        &blinded_tokens.0,
                        &signed_tokens.0,
                        &public_key.0,
                    )?
                    .into_iter()
                    .map(|t| t.into())
                    .collect::<Vec<UnblindedToken>>()
                    .into())
            }()
            .into(),
        )
    }
}

#[derive(From)]
struct BatchDLEQProofResult(Result<BatchDLEQProof, Error>);

impl_decode_base64!(decode_base64_batch_dleq_proof, BatchDLEQProof, BatchDLEQProofResult);
impl_result!(BatchDLEQProof, BatchDLEQProofResult);
