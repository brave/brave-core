/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_CHALLENGE_BYPASS_RISTRETTO_FFI_SRC_WRAPPER_H_
#define BRAVE_THIRD_PARTY_CHALLENGE_BYPASS_RISTRETTO_FFI_SRC_WRAPPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/types/expected.h"

extern "C" {
#include "brave/third_party/challenge_bypass_ristretto_ffi/src/lib.h"
}

#if defined(CHALLENGE_BYPASS_RISTRETTO_SHARED_LIBRARY)
#if defined(WIN32)
#if defined(CHALLENGE_BYPASS_RISTRETTO_IMPLEMENTATION)
#define CHALLENGE_BYPASS_RISTRETTO_EXPORT __declspec(dllexport)
#else
#define CHALLENGE_BYPASS_RISTRETTO_EXPORT __declspec(dllimport)
#endif  // defined(CHALLENGE_BYPASS_RISTRETTO_IMPLEMENTATION)
#else   // defined(WIN32)
#if defined(CHALLENGE_BYPASS_RISTRETTO_IMPLEMENTATION)
#define CHALLENGE_BYPASS_RISTRETTO_EXPORT __attribute__((visibility("default")))
#else
#define CHALLENGE_BYPASS_RISTRETTO_EXPORT
#endif
#endif
#else  // defined(CHALLENGE_BYPASS_RISTRETTO_SHARED_LIBRARY)
#define CHALLENGE_BYPASS_RISTRETTO_EXPORT
#endif

namespace challenge_bypass_ristretto {

class CHALLENGE_BYPASS_RISTRETTO_EXPORT TokenPreimage {
  friend class SigningKey;

 public:
  explicit TokenPreimage(std::shared_ptr<C_TokenPreimage>);
  TokenPreimage(const TokenPreimage&);
  ~TokenPreimage();
  static base::expected<TokenPreimage, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;

  bool operator==(const TokenPreimage& rhs) const;
  bool operator!=(const TokenPreimage& rhs) const;

 private:
  std::shared_ptr<C_TokenPreimage> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT BlindedToken {
  friend class SigningKey;
  friend class DLEQProof;
  friend class BatchDLEQProof;

 public:
  explicit BlindedToken(std::shared_ptr<C_BlindedToken>);
  BlindedToken(const BlindedToken& other);
  BlindedToken& operator=(const BlindedToken& other);
  BlindedToken(BlindedToken&& other);
  BlindedToken& operator=(BlindedToken&& other);
  ~BlindedToken();
  static base::expected<BlindedToken, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;

  bool operator==(const BlindedToken& rhs) const;
  bool operator!=(const BlindedToken& rhs) const;

 private:
  std::shared_ptr<C_BlindedToken> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT SignedToken {
  friend class Token;
  friend class DLEQProof;
  friend class BatchDLEQProof;

 public:
  explicit SignedToken(std::shared_ptr<C_SignedToken>);
  SignedToken(const SignedToken&);
  ~SignedToken();
  static base::expected<SignedToken, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;

  bool operator==(const SignedToken& rhs) const;
  bool operator!=(const SignedToken& rhs) const;

 private:
  std::shared_ptr<C_SignedToken> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT VerificationSignature {
  friend class VerificationKey;

 public:
  explicit VerificationSignature(std::shared_ptr<C_VerificationSignature>);
  VerificationSignature(const VerificationSignature&);
  ~VerificationSignature();
  static base::expected<VerificationSignature, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;

 private:
  std::shared_ptr<C_VerificationSignature> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT VerificationKey {
 public:
  explicit VerificationKey(std::shared_ptr<C_VerificationKey>);
  VerificationKey(const VerificationKey&);
  ~VerificationKey();
  base::expected<VerificationSignature, std::string> sign(
      base::span<const uint8_t>);
  base::expected<bool, std::string> verify(VerificationSignature,
                                           base::span<const uint8_t>);

 private:
  std::shared_ptr<C_VerificationKey> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT UnblindedToken {
 public:
  explicit UnblindedToken(std::shared_ptr<C_UnblindedToken>);
  UnblindedToken(const UnblindedToken&);
  ~UnblindedToken();
  VerificationKey derive_verification_key() const;
  TokenPreimage preimage() const;
  static base::expected<UnblindedToken, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;

  bool operator==(const UnblindedToken& rhs) const;
  bool operator!=(const UnblindedToken& rhs) const;

 private:
  std::shared_ptr<C_UnblindedToken> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT Token {
  friend class BatchDLEQProof;

 public:
  explicit Token(std::shared_ptr<C_Token>);
  Token(const Token&);
  ~Token();
  static base::expected<Token, std::string> random();
  base::expected<BlindedToken, std::string> blind();
  static base::expected<Token, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;

  bool operator==(const Token& rhs) const;
  bool operator!=(const Token& rhs) const;

 private:
  std::shared_ptr<C_Token> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT PublicKey {
  friend class DLEQProof;
  friend class BatchDLEQProof;

 public:
  explicit PublicKey(std::shared_ptr<C_PublicKey>);
  PublicKey(const PublicKey&);
  ~PublicKey();
  static base::expected<PublicKey, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;

  bool operator==(const PublicKey& rhs) const;
  bool operator!=(const PublicKey& rhs) const;

 private:
  std::shared_ptr<C_PublicKey> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT SigningKey {
  friend class DLEQProof;
  friend class BatchDLEQProof;

 public:
  explicit SigningKey(std::shared_ptr<C_SigningKey>);
  SigningKey(const SigningKey&);
  ~SigningKey();
  static base::expected<SigningKey, std::string> random();
  base::expected<SignedToken, std::string> sign(BlindedToken) const;
  UnblindedToken rederive_unblinded_token(TokenPreimage);
  PublicKey public_key();
  static base::expected<SigningKey, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;

  bool operator==(const SigningKey& rhs) const;
  bool operator!=(const SigningKey& rhs) const;

 private:
  std::shared_ptr<C_SigningKey> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT DLEQProof {
 public:
  explicit DLEQProof(std::shared_ptr<C_DLEQProof>);
  DLEQProof(const DLEQProof&);
  ~DLEQProof();
  base::expected<bool, std::string> verify(BlindedToken,
                                           SignedToken,
                                           PublicKey);
  static base::expected<DLEQProof, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;

  bool operator==(const DLEQProof& rhs) const;
  bool operator!=(const DLEQProof& rhs) const;

  static base::expected<DLEQProof, std::string>
  Create(BlindedToken blinded_token, SignedToken signed_token, SigningKey key);

 private:
  DLEQProof(BlindedToken, SignedToken, SigningKey);
  std::shared_ptr<C_DLEQProof> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT BatchDLEQProof {
 public:
  explicit BatchDLEQProof(std::shared_ptr<C_BatchDLEQProof>);
  BatchDLEQProof(const BatchDLEQProof&);
  ~BatchDLEQProof();
  base::expected<bool, std::string> verify(std::vector<BlindedToken>,
                                           std::vector<SignedToken>,
                                           PublicKey);
  base::expected<std::vector<UnblindedToken>, std::string> verify_and_unblind(
      std::vector<Token>,
      std::vector<BlindedToken>,
      std::vector<SignedToken>,
      PublicKey);
  static base::expected<BatchDLEQProof, std::string> decode_base64(
      base::span<const uint8_t>);
  base::expected<std::string, std::string> encode_base64() const;
  static base::expected<BatchDLEQProof, std::string> Create(
      std::vector<BlindedToken> blinded_tokens,
      std::vector<SignedToken> signed_tokens,
      SigningKey key);

  bool operator==(const BatchDLEQProof& rhs) const;
  bool operator!=(const BatchDLEQProof& rhs) const;

 private:
  BatchDLEQProof(std::vector<BlindedToken>,
                 std::vector<SignedToken>,
                 SigningKey);

  std::shared_ptr<C_BatchDLEQProof> raw;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_THIRD_PARTY_CHALLENGE_BYPASS_RISTRETTO_FFI_SRC_WRAPPER_H_
