#ifndef _CHALLENGE_BYPASS_RISTRETTO_WRAPPER_HPP
#define _CHALLENGE_BYPASS_RISTRETTO_WRAPPER_HPP

#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "lib.h"
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

class CHALLENGE_BYPASS_RISTRETTO_EXPORT TokenException : std::exception {
 public:
  TokenException(const std::string& msg);
  ~TokenException() override;
  static TokenException last_error(std::string msg);
  const char* what() const noexcept override;
#ifdef NO_CXXEXCEPTIONS
  static const TokenException& none();
  static void set_last_exception(const TokenException& exception);
  bool is_empty() const;
#endif

 private:
  std::string msg_;
};

#ifdef NO_CXXEXCEPTIONS
CHALLENGE_BYPASS_RISTRETTO_EXPORT bool exception_occurred();
CHALLENGE_BYPASS_RISTRETTO_EXPORT const TokenException get_last_exception();
#endif

class CHALLENGE_BYPASS_RISTRETTO_EXPORT TokenPreimage {
  friend class SigningKey;

 public:
  TokenPreimage(std::shared_ptr<C_TokenPreimage>);
  TokenPreimage(const TokenPreimage&);
  ~TokenPreimage();
  static TokenPreimage decode_base64(const std::string);
  std::string encode_base64() const;

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
  BlindedToken(std::shared_ptr<C_BlindedToken>);
  BlindedToken(const BlindedToken&);
  ~BlindedToken();
  static BlindedToken decode_base64(const std::string);
  std::string encode_base64() const;

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
  SignedToken(std::shared_ptr<C_SignedToken>);
  SignedToken(const SignedToken&);
  ~SignedToken();
  static SignedToken decode_base64(const std::string);
  std::string encode_base64() const;

  bool operator==(const SignedToken& rhs) const;
  bool operator!=(const SignedToken& rhs) const;

 private:
  std::shared_ptr<C_SignedToken> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT VerificationSignature {
  friend class VerificationKey;

 public:
  VerificationSignature(std::shared_ptr<C_VerificationSignature>);
  VerificationSignature(const VerificationSignature&);
  ~VerificationSignature();
  static VerificationSignature decode_base64(const std::string);
  std::string encode_base64() const;

 private:
  std::shared_ptr<C_VerificationSignature> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT VerificationKey {
 public:
  VerificationKey(std::shared_ptr<C_VerificationKey>);
  VerificationKey(const VerificationKey&);
  ~VerificationKey();
  VerificationSignature sign(const std::string);
  bool verify(VerificationSignature, const std::string);

 private:
  std::shared_ptr<C_VerificationKey> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT UnblindedToken {
 public:
  UnblindedToken(std::shared_ptr<C_UnblindedToken>);
  UnblindedToken(const UnblindedToken&);
  ~UnblindedToken();
  VerificationKey derive_verification_key() const;
  TokenPreimage preimage() const;
  static UnblindedToken decode_base64(const std::string);
  std::string encode_base64() const;

  bool operator==(const UnblindedToken& rhs) const;
  bool operator!=(const UnblindedToken& rhs) const;

 private:
  std::shared_ptr<C_UnblindedToken> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT Token {
  friend class BatchDLEQProof;

 public:
  Token(std::shared_ptr<C_Token>);
  Token(const Token&);
  ~Token();
  static Token random();
  BlindedToken blind();
  static Token decode_base64(const std::string);
  std::string encode_base64() const;

  bool operator==(const Token& rhs) const;
  bool operator!=(const Token& rhs) const;

 private:
  std::shared_ptr<C_Token> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT PublicKey {
  friend class DLEQProof;
  friend class BatchDLEQProof;

 public:
  PublicKey(std::shared_ptr<C_PublicKey>);
  PublicKey(const PublicKey&);
  ~PublicKey();
  static PublicKey decode_base64(const std::string);
  std::string encode_base64() const;

  bool operator==(const PublicKey& rhs) const;
  bool operator!=(const PublicKey& rhs) const;

 private:
  std::shared_ptr<C_PublicKey> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT SigningKey {
  friend class DLEQProof;
  friend class BatchDLEQProof;

 public:
  SigningKey(std::shared_ptr<C_SigningKey>);
  SigningKey(const SigningKey&);
  ~SigningKey();
  static SigningKey random();
  SignedToken sign(BlindedToken) const;
  UnblindedToken rederive_unblinded_token(TokenPreimage);
  PublicKey public_key();
  static SigningKey decode_base64(const std::string);
  std::string encode_base64() const;

  bool operator==(const SigningKey& rhs) const;
  bool operator!=(const SigningKey& rhs) const;

 private:
  std::shared_ptr<C_SigningKey> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT DLEQProof {
 public:
  DLEQProof(std::shared_ptr<C_DLEQProof>);
  DLEQProof(const DLEQProof&);
  DLEQProof(BlindedToken, SignedToken, SigningKey);
  ~DLEQProof();
  bool verify(BlindedToken, SignedToken, PublicKey);
  static DLEQProof decode_base64(const std::string);
  std::string encode_base64() const;

  bool operator==(const DLEQProof& rhs) const;
  bool operator!=(const DLEQProof& rhs) const;

 private:
  std::shared_ptr<C_DLEQProof> raw;
};

class CHALLENGE_BYPASS_RISTRETTO_EXPORT BatchDLEQProof {
 public:
  BatchDLEQProof(std::shared_ptr<C_BatchDLEQProof>);
  BatchDLEQProof(const BatchDLEQProof&);
  BatchDLEQProof(std::vector<BlindedToken>,
                 std::vector<SignedToken>,
                 SigningKey);
  ~BatchDLEQProof();
  bool verify(std::vector<BlindedToken>, std::vector<SignedToken>, PublicKey);
  std::vector<UnblindedToken> verify_and_unblind(std::vector<Token>,
                                                 std::vector<BlindedToken>,
                                                 std::vector<SignedToken>,
                                                 PublicKey);
  static BatchDLEQProof decode_base64(const std::string);
  std::string encode_base64() const;

  bool operator==(const BatchDLEQProof& rhs) const;
  bool operator!=(const BatchDLEQProof& rhs) const;

 private:
  std::shared_ptr<C_BatchDLEQProof> raw;
};

}  // namespace challenge_bypass_ristretto

#endif /* _CHALLENGE_BYPASS_RISTRETTO_WRAPPER_HPP */
