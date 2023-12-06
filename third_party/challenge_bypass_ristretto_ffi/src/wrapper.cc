/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/challenge_bypass_ristretto_ffi/src/wrapper.h"

#include "base/containers/span.h"

extern "C" {
#include "brave/third_party/challenge_bypass_ristretto_ffi/src/lib.h"
}

// class TokenPreimage
namespace challenge_bypass_ristretto {
TokenPreimage::TokenPreimage(std::shared_ptr<C_TokenPreimage> raw) : raw(raw) {}
TokenPreimage::TokenPreimage(const TokenPreimage& other) = default;
TokenPreimage::~TokenPreimage() {}

base::expected<TokenPreimage, std::string> TokenPreimage::decode_base64(
    base::span<const uint8_t> encoded) {
  std::shared_ptr<C_TokenPreimage> raw_preimage(
      token_preimage_decode_base64(encoded.data(), encoded.size()),
      token_preimage_destroy);
  if (raw_preimage == nullptr) {
    return base::unexpected("Failed to decode token preimage");
  }
  return TokenPreimage(raw_preimage);
}

base::expected<std::string, std::string> TokenPreimage::encode_base64() const {
  char* tmp = token_preimage_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode token preimage");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

bool TokenPreimage::operator==(const TokenPreimage& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool TokenPreimage::operator!=(const TokenPreimage& rhs) const {
  return !(*this == rhs);
}

Token::Token(std::shared_ptr<C_Token> raw) : raw(raw) {}
Token::Token(const Token& other) = default;
Token::~Token() {}

base::expected<Token, std::string> Token::random() {
  std::shared_ptr<C_Token> raw_token(token_random(), token_destroy);
  if (raw_token == nullptr) {
    return base::unexpected("Failed to generate random token");
  }
  return Token(raw_token);
}

base::expected<BlindedToken, std::string> Token::blind() {
  std::shared_ptr<C_BlindedToken> raw_blinded(token_blind(raw.get()),
                                              blinded_token_destroy);
  if (raw_blinded == nullptr) {
    return base::unexpected("Failed to blind");
  }

  return BlindedToken(raw_blinded);
}

base::expected<Token, std::string> Token::decode_base64(
    base::span<const uint8_t> encoded) {
  std::shared_ptr<C_Token> raw_tok(
      token_decode_base64(encoded.data(), encoded.size()), token_destroy);
  if (raw_tok == nullptr) {
    return base::unexpected("Failed to decode token");
  }
  return Token(raw_tok);
}

base::expected<std::string, std::string> Token::encode_base64() const {
  char* tmp = token_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode token");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

bool Token::operator==(const Token& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool Token::operator!=(const Token& rhs) const {
  return !(*this == rhs);
}

BlindedToken::BlindedToken(std::shared_ptr<C_BlindedToken> raw) : raw(raw) {}
BlindedToken::BlindedToken(BlindedToken&& other) = default;
BlindedToken& BlindedToken::operator=(BlindedToken&& other) = default;
BlindedToken::BlindedToken(const BlindedToken& other) = default;
BlindedToken& BlindedToken::operator=(const BlindedToken& other) = default;
BlindedToken::~BlindedToken() {}

base::expected<BlindedToken, std::string> BlindedToken::decode_base64(
    base::span<const uint8_t> encoded) {
  std::shared_ptr<C_BlindedToken> raw_blinded(
      blinded_token_decode_base64(encoded.data(), encoded.size()),
      blinded_token_destroy);
  if (raw_blinded == nullptr) {
    return base::unexpected("Failed to decode blinded token");
  }
  return BlindedToken(raw_blinded);
}

base::expected<std::string, std::string> BlindedToken::encode_base64() const {
  char* tmp = blinded_token_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode blinded token");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

bool BlindedToken::operator==(const BlindedToken& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool BlindedToken::operator!=(const BlindedToken& rhs) const {
  return !(*this == rhs);
}

SignedToken::SignedToken(std::shared_ptr<C_SignedToken> raw) : raw(raw) {}
SignedToken::SignedToken(const SignedToken& other) = default;
SignedToken::~SignedToken() {}

base::expected<SignedToken, std::string> SignedToken::decode_base64(
    base::span<const uint8_t> encoded) {
  std::shared_ptr<C_SignedToken> raw_signed(
      signed_token_decode_base64(encoded.data(), encoded.size()),
      signed_token_destroy);
  if (raw_signed == nullptr) {
    return base::unexpected("Failed to decode signed token");
  }
  return SignedToken(raw_signed);
}

base::expected<std::string, std::string> SignedToken::encode_base64() const {
  char* tmp = signed_token_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode signed token");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

bool SignedToken::operator==(const SignedToken& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool SignedToken::operator!=(const SignedToken& rhs) const {
  return !(*this == rhs);
}

VerificationSignature::VerificationSignature(
    std::shared_ptr<C_VerificationSignature> raw)
    : raw(raw) {}
VerificationSignature::VerificationSignature(
    const VerificationSignature& other) = default;
VerificationSignature::~VerificationSignature() {}

base::expected<VerificationSignature, std::string>
VerificationSignature::decode_base64(base::span<const uint8_t> encoded) {
  std::shared_ptr<C_VerificationSignature> raw_sig(
      verification_signature_decode_base64(encoded.data(), encoded.size()),
      verification_signature_destroy);
  if (raw_sig == nullptr) {
    return base::unexpected("Failed to decode verification signature");
  }
  return VerificationSignature(raw_sig);
}

base::expected<std::string, std::string> VerificationSignature::encode_base64()
    const {
  char* tmp = verification_signature_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode verification signature");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

UnblindedToken::UnblindedToken(std::shared_ptr<C_UnblindedToken> raw)
    : raw(raw) {}
UnblindedToken::UnblindedToken(const UnblindedToken& other) = default;
UnblindedToken::~UnblindedToken() {}

VerificationKey UnblindedToken::derive_verification_key() const {
  return VerificationKey(std::shared_ptr<C_VerificationKey>(
      unblinded_token_derive_verification_key_sha512(raw.get()),
      verification_key_destroy));
}

TokenPreimage UnblindedToken::preimage() const {
  return TokenPreimage(std::shared_ptr<C_TokenPreimage>(
      unblinded_token_preimage(raw.get()), token_preimage_destroy));
}

base::expected<UnblindedToken, std::string> UnblindedToken::decode_base64(
    base::span<const uint8_t> encoded) {
  std::shared_ptr<C_UnblindedToken> raw_unblinded(
      unblinded_token_decode_base64(encoded.data(), encoded.size()),
      unblinded_token_destroy);
  if (raw_unblinded == nullptr) {
    return base::unexpected("Failed to decode unblinded token");
  }
  return UnblindedToken(raw_unblinded);
}

base::expected<std::string, std::string> UnblindedToken::encode_base64() const {
  char* tmp = unblinded_token_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode unblinded token");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

bool UnblindedToken::operator==(const UnblindedToken& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool UnblindedToken::operator!=(const UnblindedToken& rhs) const {
  return !(*this == rhs);
}

VerificationKey::VerificationKey(std::shared_ptr<C_VerificationKey> raw)
    : raw(raw) {}
VerificationKey::VerificationKey(const VerificationKey& other) = default;
VerificationKey::~VerificationKey() {}

base::expected<VerificationSignature, std::string> VerificationKey::sign(
    base::span<const uint8_t> message) {
  std::shared_ptr<C_VerificationSignature> raw_verification_signature(
      verification_key_sign_sha512(raw.get(), message.data(), message.size()),
      verification_signature_destroy);
  if (raw_verification_signature == nullptr) {
    base::unexpected("Failed to sign message");
  }
  return VerificationSignature(raw_verification_signature);
}

base::expected<bool, std::string> VerificationKey::verify(
    VerificationSignature sig,
    base::span<const uint8_t> message) {
  int result = verification_key_invalid_sha512(raw.get(), sig.raw.get(),
                                               message.data(), message.size());
  if (result < 0) {
    base::unexpected("Failed to verify message signature");
  }
  return result == 0;
}

SigningKey::SigningKey(std::shared_ptr<C_SigningKey> raw) : raw(raw) {}
SigningKey::SigningKey(const SigningKey& other) = default;
SigningKey::~SigningKey() {}

base::expected<SigningKey, std::string> SigningKey::random() {
  std::shared_ptr<C_SigningKey> raw_key(signing_key_random(),
                                        signing_key_destroy);
  if (raw_key == nullptr) {
    base::unexpected("Failed to generate random signing key");
  }
  return SigningKey(raw_key);
}

base::expected<SignedToken, std::string> SigningKey::sign(
    BlindedToken tok) const {
  std::shared_ptr<C_SignedToken> raw_signed(
      signing_key_sign(raw.get(), tok.raw.get()), signed_token_destroy);
  if (raw_signed == nullptr) {
    base::unexpected("Failed to sign blinded token");
  }

  return SignedToken(raw_signed);
}

UnblindedToken SigningKey::rederive_unblinded_token(TokenPreimage t) {
  return UnblindedToken(std::shared_ptr<C_UnblindedToken>(
      signing_key_rederive_unblinded_token(raw.get(), t.raw.get()),
      unblinded_token_destroy));
}

PublicKey SigningKey::public_key() {
  return PublicKey(std::shared_ptr<C_PublicKey>(
      signing_key_get_public_key(raw.get()), public_key_destroy));
}

base::expected<SigningKey, std::string> SigningKey::decode_base64(
    base::span<const uint8_t> encoded) {
  std::shared_ptr<C_SigningKey> raw_key(
      signing_key_decode_base64(encoded.data(), encoded.size()),
      signing_key_destroy);
  if (raw_key == nullptr) {
    return base::unexpected("Failed to decode signing key");
  }
  return SigningKey(raw_key);
}

base::expected<std::string, std::string> SigningKey::encode_base64() const {
  char* tmp = signing_key_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode signing key");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

bool SigningKey::operator==(const SigningKey& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool SigningKey::operator!=(const SigningKey& rhs) const {
  return !(*this == rhs);
}

PublicKey::PublicKey(std::shared_ptr<C_PublicKey> raw) : raw(raw) {}
PublicKey::PublicKey(const PublicKey& other) = default;
PublicKey::~PublicKey() {}

base::expected<PublicKey, std::string> PublicKey::decode_base64(
    base::span<const uint8_t> encoded) {
  std::shared_ptr<C_PublicKey> raw_key(
      public_key_decode_base64(encoded.data(), encoded.size()),
      public_key_destroy);
  if (raw_key == nullptr) {
    return base::unexpected("Failed to decode public key");
  }
  return PublicKey(raw_key);
}

base::expected<std::string, std::string> PublicKey::encode_base64() const {
  char* tmp = public_key_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode public key");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

bool PublicKey::operator==(const PublicKey& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool PublicKey::operator!=(const PublicKey& rhs) const {
  return !(*this == rhs);
}

DLEQProof::DLEQProof(std::shared_ptr<C_DLEQProof> raw) : raw(raw) {}
DLEQProof::DLEQProof(const DLEQProof& other) = default;
DLEQProof::~DLEQProof() {}

DLEQProof::DLEQProof(BlindedToken blinded_token,
                     SignedToken signed_token,
                     SigningKey key) {
  raw = std::shared_ptr<C_DLEQProof>(
      dleq_proof_new(blinded_token.raw.get(), signed_token.raw.get(),
                     key.raw.get()),
      dleq_proof_destroy);
}

// static
base::expected<DLEQProof, std::string> DLEQProof::Create(
    BlindedToken blinded_token,
    SignedToken signed_token,
    SigningKey key) {
  DLEQProof proof(blinded_token, signed_token, key);
  if (proof.raw == nullptr) {
    return base::unexpected("Failed to create new DLEQ proof");
  }

  return proof;
}

base::expected<bool, std::string> DLEQProof::verify(BlindedToken blinded_token,
                                                    SignedToken signed_token,
                                                    PublicKey key) {
  int result = dleq_proof_invalid(raw.get(), blinded_token.raw.get(),
                                  signed_token.raw.get(), key.raw.get());
  if (result < 0) {
    return base::unexpected("Failed to verify DLEQ proof");
  }
  return result == 0;
}

base::expected<DLEQProof, std::string> DLEQProof::decode_base64(
    base::span<const uint8_t> encoded) {
  std::shared_ptr<C_DLEQProof> raw_proof(
      dleq_proof_decode_base64(encoded.data(), encoded.size()),
      dleq_proof_destroy);
  if (raw_proof == nullptr) {
    return base::unexpected("Failed to decode DLEQ proof");
  }
  return DLEQProof(raw_proof);
}

base::expected<std::string, std::string> DLEQProof::encode_base64() const {
  char* tmp = dleq_proof_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode DLEQ proof");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

bool DLEQProof::operator==(const DLEQProof& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool DLEQProof::operator!=(const DLEQProof& rhs) const {
  return !(*this == rhs);
}

BatchDLEQProof::BatchDLEQProof(std::shared_ptr<C_BatchDLEQProof> raw)
    : raw(raw) {}
BatchDLEQProof::BatchDLEQProof(const BatchDLEQProof& other) = default;
BatchDLEQProof::~BatchDLEQProof() {}

BatchDLEQProof::BatchDLEQProof(std::vector<BlindedToken> blinded_tokens,
                               std::vector<SignedToken> signed_tokens,
                               SigningKey key) {
  std::vector<C_BlindedToken*> raw_blinded_tokens;
  std::vector<C_SignedToken*> raw_signed_tokens;

  for (size_t i = 0; i < blinded_tokens.size(); i++) {
    raw_blinded_tokens.push_back(blinded_tokens[i].raw.get());
    raw_signed_tokens.push_back(signed_tokens[i].raw.get());
  }

  raw = std::shared_ptr<C_BatchDLEQProof>(
      batch_dleq_proof_new(raw_blinded_tokens.data(), raw_signed_tokens.data(),
                           blinded_tokens.size(), key.raw.get()),
      batch_dleq_proof_destroy);
}

// static
base::expected<BatchDLEQProof, std::string> BatchDLEQProof::Create(
    std::vector<BlindedToken> blinded_tokens,
    std::vector<SignedToken> signed_tokens,
    SigningKey key) {
  if (blinded_tokens.size() != signed_tokens.size()) {
    return base::unexpected(
        "Blinded tokens and signed tokens must have the same length");
  }
  BatchDLEQProof proof(blinded_tokens, signed_tokens, key);
  if (proof.raw == nullptr) {
    return base::unexpected("Failed to create new batch DLEQ proof");
  }
  return proof;
}

base::expected<bool, std::string> BatchDLEQProof::verify(
    std::vector<BlindedToken> blinded_tokens,
    std::vector<SignedToken> signed_tokens,
    PublicKey key) {
  if (blinded_tokens.size() != signed_tokens.size()) {
    return base::unexpected(
        "Blinded tokens and signed tokens must have the same length");
  }
  std::vector<C_BlindedToken*> raw_blinded_tokens;
  std::vector<C_SignedToken*> raw_signed_tokens;

  for (size_t i = 0; i < blinded_tokens.size(); i++) {
    raw_blinded_tokens.push_back(blinded_tokens[i].raw.get());
    raw_signed_tokens.push_back(signed_tokens[i].raw.get());
  }

  int result = batch_dleq_proof_invalid(raw.get(), raw_blinded_tokens.data(),
                                        raw_signed_tokens.data(),
                                        blinded_tokens.size(), key.raw.get());
  if (result < 0) {
    return base::unexpected("Could not verify DLEQ proof");
  }
  return result == 0;
}

base::expected<std::vector<UnblindedToken>, std::string>
BatchDLEQProof::verify_and_unblind(std::vector<Token> tokens,
                                   std::vector<BlindedToken> blinded_tokens,
                                   std::vector<SignedToken> signed_tokens,
                                   PublicKey public_key) {
  std::vector<UnblindedToken> unblinded_tokens;

  if (tokens.size() != blinded_tokens.size() ||
      tokens.size() != signed_tokens.size()) {
    return base::unexpected(
        "Tokens, blinded tokens and signed tokens must have the same length");
  }

  std::vector<C_Token*> raw_tokens;
  std::vector<C_BlindedToken*> raw_blinded_tokens;
  std::vector<C_SignedToken*> raw_signed_tokens;
  std::vector<C_UnblindedToken*> raw_unblinded_tokens(tokens.size());

  for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i].raw.get()) {
      raw_tokens.push_back(tokens[i].raw.get());
    }

    if (blinded_tokens[i].raw.get()) {
      raw_blinded_tokens.push_back(blinded_tokens[i].raw.get());
    }

    if (signed_tokens[i].raw.get()) {
      raw_signed_tokens.push_back(signed_tokens[i].raw.get());
    }
  }

  if (raw_tokens.size() != raw_blinded_tokens.size() ||
      raw_tokens.size() != raw_signed_tokens.size()) {
    return base::unexpected(
        "Raw tokens, raw blinded tokens and raw signed tokens "
        "must have the same length");
  }

  if (!public_key.raw.get()) {
    return base::unexpected("Could not verify DLEQ proof");
  }

  int result = batch_dleq_proof_invalid_or_unblind(
      raw.get(), raw_tokens.data(), raw_blinded_tokens.data(),
      raw_signed_tokens.data(), raw_unblinded_tokens.data(), tokens.size(),
      public_key.raw.get());
  if (result != 0) {
    if (result < 0) {
      return base::unexpected("Could not verify DLEQ proof");
    }
    return unblinded_tokens;
  }

  for (size_t i = 0; i < tokens.size(); i++) {
    std::shared_ptr<C_UnblindedToken> raw_unblinded(raw_unblinded_tokens.at(i),
                                                    unblinded_token_destroy);

    if (raw_unblinded == nullptr) {
      return base::unexpected("Unexpected failure to unblind");
    }

    unblinded_tokens.push_back(UnblindedToken(raw_unblinded));
  }

  return unblinded_tokens;
}

base::expected<BatchDLEQProof, std::string> BatchDLEQProof::decode_base64(
    base::span<const uint8_t> encoded) {
  std::shared_ptr<C_BatchDLEQProof> raw_proof(
      batch_dleq_proof_decode_base64(encoded.data(), encoded.size()),
      batch_dleq_proof_destroy);
  if (raw_proof == nullptr) {
    return base::unexpected("Failed to decode batch DLEQ proof");
  }
  return BatchDLEQProof(raw_proof);
}

base::expected<std::string, std::string> BatchDLEQProof::encode_base64() const {
  char* tmp = batch_dleq_proof_encode_base64(raw.get());
  if (tmp == nullptr) {
    return base::unexpected("Failed to encode batch DLEQ proof");
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return base::ok(result);
}

bool BatchDLEQProof::operator==(const BatchDLEQProof& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool BatchDLEQProof::operator!=(const BatchDLEQProof& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto
