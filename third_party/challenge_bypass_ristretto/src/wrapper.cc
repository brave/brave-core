#include "brave/third_party/challenge_bypass_ristretto/src/wrapper.h"

extern "C" {
#include "lib.h"
}

#include <thread>

#ifdef NO_CXXEXCEPTIONS
#include "base/no_destructor.h"
#include "base/threading/thread_local.h"
#endif

#ifndef DCHECK_IS_ON
#ifdef NDEBUG
#define DCHECK_IS_ON() 1
#else
#define DCHECK_IS_ON() 0
#endif
#endif

#ifndef DCHECK
#if DCHECK_IS_ON()
#define DCHECK(expr) \
  if (!expr) {       \
    std::abort();    \
  }
#else
#define DCHECK(expr) ((void)0)
#endif
#endif

#ifdef NO_CXXEXCEPTIONS
#define THROW(expr) TokenException::set_last_exception(expr);
#define CLEAR_LAST_EXCEPTION(expr)                              \
  do {                                                          \
    TokenException::set_last_exception(TokenException::none()); \
  } while (0)
#else
#define THROW(expr) (throw expr);
#define CLEAR_LAST_EXCEPTION(expr) ((void)0);
#endif

// class TokenException
namespace challenge_bypass_ristretto {

#ifdef NO_CXXEXCEPTIONS
namespace {

TokenException* GetOrCreateLastException() {
  static base::NoDestructor<base::ThreadLocalPointer<TokenException>>
      last_exception;
  TokenException* token_exception = last_exception.get()->Get();
  if (!token_exception) {
    token_exception = new TokenException("");
    last_exception.get()->Set(token_exception);
  }
  return token_exception;
}

}  // namespace

const TokenException get_last_exception() {
  TokenException* token_exception = GetOrCreateLastException();

  TokenException tmp = *token_exception;
  TokenException::set_last_exception(TokenException::none());
  return tmp;
}

bool exception_occurred() {
  return !GetOrCreateLastException()->is_empty();
}
#endif

TokenException::TokenException(const std::string& msg) : msg_(msg) {}
TokenException::~TokenException() {}

TokenException TokenException::last_error(std::string default_msg) {
  char* tmp = last_error_message();
  if (tmp != nullptr) {
    std::string msg = std::string(tmp);
    c_char_destroy(tmp);
    return TokenException(default_msg + ": " + msg);
  } else {
    return TokenException(default_msg);
  }
}
const char* TokenException::what() const noexcept {
  return msg_.c_str();
}

#ifdef NO_CXXEXCEPTIONS
const TokenException& TokenException::none() {
  static base::NoDestructor<TokenException> token_exception_none("");
  return *token_exception_none;
}

bool TokenException::is_empty() const {
  return msg_.empty();
}

void TokenException::set_last_exception(const TokenException& exception) {
  TokenException* token_exception = GetOrCreateLastException();
  token_exception->msg_ = exception.msg_;
}
#endif  // NO_CXXEXCEPTIONS
}  // namespace challenge_bypass_ristretto

// class TokenPreimage
namespace challenge_bypass_ristretto {
TokenPreimage::TokenPreimage(std::shared_ptr<C_TokenPreimage> raw) : raw(raw) {}
TokenPreimage::TokenPreimage(const TokenPreimage& other) = default;
TokenPreimage::~TokenPreimage() {}

TokenPreimage TokenPreimage::decode_base64(const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_TokenPreimage> raw_preimage(
      token_preimage_decode_base64((const uint8_t*)encoded.data(),
                                   encoded.length()),
      token_preimage_destroy);
  if (raw_preimage == nullptr) {
    THROW(TokenException::last_error("Failed to decode token preimage"));
  }
  return TokenPreimage(raw_preimage);
}

std::string TokenPreimage::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = token_preimage_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(TokenException::last_error("Failed to encode token preimage"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}

bool TokenPreimage::operator==(const TokenPreimage& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool TokenPreimage::operator!=(const TokenPreimage& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto

// class Token
namespace challenge_bypass_ristretto {
Token::Token(std::shared_ptr<C_Token> raw) : raw(raw) {}
Token::Token(const Token& other) = default;
Token::~Token() {}

Token Token::random() {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_Token> raw_token(token_random(), token_destroy);
  if (raw_token == nullptr) {
    THROW(TokenException::last_error("Failed to generate random token"));
  }
  return Token(raw_token);
}

BlindedToken Token::blind() {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_BlindedToken> raw_blinded(token_blind(raw.get()),
                                              blinded_token_destroy);
  if (raw_blinded == nullptr) {
    THROW(TokenException::last_error("Failed to blind"));
  }

  return BlindedToken(raw_blinded);
}

Token Token::decode_base64(const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_Token> raw_tok(
      token_decode_base64((const uint8_t*)encoded.data(), encoded.length()),
      token_destroy);
  if (raw_tok == nullptr) {
    THROW(TokenException::last_error("Failed to decode token"));
  }
  return Token(raw_tok);
}

std::string Token::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = token_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(TokenException::last_error("Failed to encode token"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}

bool Token::operator==(const Token& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool Token::operator!=(const Token& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto

// class BlindedToken
namespace challenge_bypass_ristretto {
BlindedToken::BlindedToken(std::shared_ptr<C_BlindedToken> raw) : raw(raw) {}
BlindedToken::BlindedToken(const BlindedToken& other) = default;
BlindedToken::~BlindedToken() {}

BlindedToken BlindedToken::decode_base64(const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_BlindedToken> raw_blinded(
      blinded_token_decode_base64((const uint8_t*)encoded.data(),
                                  encoded.length()),
      blinded_token_destroy);
  if (raw_blinded == nullptr) {
    THROW(TokenException::last_error("Failed to decode blinded token"));
  }
  return BlindedToken(raw_blinded);
}

std::string BlindedToken::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = blinded_token_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(TokenException::last_error("Failed to encode blinded token"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}

bool BlindedToken::operator==(const BlindedToken& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool BlindedToken::operator!=(const BlindedToken& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto

// class SignedToken
namespace challenge_bypass_ristretto {
SignedToken::SignedToken(std::shared_ptr<C_SignedToken> raw) : raw(raw) {}
SignedToken::SignedToken(const SignedToken& other) = default;
SignedToken::~SignedToken() {}

SignedToken SignedToken::decode_base64(const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_SignedToken> raw_signed(
      signed_token_decode_base64((const uint8_t*)encoded.data(),
                                 encoded.length()),
      signed_token_destroy);
  if (raw_signed == nullptr) {
    THROW(TokenException::last_error("Failed to decode signed token"));
  }
  return SignedToken(raw_signed);
}

std::string SignedToken::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = signed_token_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(TokenException::last_error("Failed to encode signed token"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}

bool SignedToken::operator==(const SignedToken& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool SignedToken::operator!=(const SignedToken& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto

// class VerificationSignature
namespace challenge_bypass_ristretto {
VerificationSignature::VerificationSignature(
    std::shared_ptr<C_VerificationSignature> raw)
    : raw(raw) {}
VerificationSignature::VerificationSignature(
    const VerificationSignature& other) = default;
VerificationSignature::~VerificationSignature() {}

VerificationSignature VerificationSignature::decode_base64(
    const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_VerificationSignature> raw_sig(
      verification_signature_decode_base64((const uint8_t*)encoded.data(),
                                           encoded.length()),
      verification_signature_destroy);
  if (raw_sig == nullptr) {
    THROW(
        TokenException::last_error("Failed to decode verification signature"));
  }
  return VerificationSignature(raw_sig);
}

std::string VerificationSignature::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = verification_signature_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(
        TokenException::last_error("Failed to encode verification signature"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}
}  // namespace challenge_bypass_ristretto

// class UnblindedToken
namespace challenge_bypass_ristretto {
UnblindedToken::UnblindedToken(std::shared_ptr<C_UnblindedToken> raw)
    : raw(raw) {}
UnblindedToken::UnblindedToken(const UnblindedToken& other) = default;
UnblindedToken::~UnblindedToken() {}

VerificationKey UnblindedToken::derive_verification_key() const {
  CLEAR_LAST_EXCEPTION();
  return VerificationKey(std::shared_ptr<C_VerificationKey>(
      unblinded_token_derive_verification_key_sha512(raw.get()),
      verification_key_destroy));
}

TokenPreimage UnblindedToken::preimage() const {
  CLEAR_LAST_EXCEPTION();
  return TokenPreimage(std::shared_ptr<C_TokenPreimage>(
      unblinded_token_preimage(raw.get()), token_preimage_destroy));
}

UnblindedToken UnblindedToken::decode_base64(const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_UnblindedToken> raw_unblinded(
      unblinded_token_decode_base64((const uint8_t*)encoded.data(),
                                    encoded.length()),
      unblinded_token_destroy);
  if (raw_unblinded == nullptr) {
    THROW(TokenException::last_error("Failed to decode unblinded token"));
  }
  return UnblindedToken(raw_unblinded);
}

std::string UnblindedToken::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = unblinded_token_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(TokenException::last_error("Failed to encode unblinded token"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}

bool UnblindedToken::operator==(const UnblindedToken& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool UnblindedToken::operator!=(const UnblindedToken& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto

// class VerificationKey
namespace challenge_bypass_ristretto {
VerificationKey::VerificationKey(std::shared_ptr<C_VerificationKey> raw)
    : raw(raw) {}
VerificationKey::VerificationKey(const VerificationKey& other) = default;
VerificationKey::~VerificationKey() {}

VerificationSignature VerificationKey::sign(const std::string message) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_VerificationSignature> raw_verification_signature(
      verification_key_sign_sha512(raw.get(), (const uint8_t*)message.data(),
                                   message.length()),
      verification_signature_destroy);
  if (raw_verification_signature == nullptr) {
    THROW(TokenException::last_error("Failed to sign message"));
  }
  return VerificationSignature(raw_verification_signature);
}

bool VerificationKey::verify(VerificationSignature sig,
                             const std::string message) {
  CLEAR_LAST_EXCEPTION();
  int result = verification_key_invalid_sha512(raw.get(), sig.raw.get(),
                                               (const uint8_t*)message.data(),
                                               message.length());
  if (result < 0) {
    THROW(TokenException::last_error("Failed to verify message signature"));
  }
  return result == 0;
}
}  // namespace challenge_bypass_ristretto

// class SigningKey
namespace challenge_bypass_ristretto {
SigningKey::SigningKey(std::shared_ptr<C_SigningKey> raw) : raw(raw) {}
SigningKey::SigningKey(const SigningKey& other) = default;
SigningKey::~SigningKey() {}

SigningKey SigningKey::random() {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_SigningKey> raw_key(signing_key_random(),
                                        signing_key_destroy);
  if (raw_key == nullptr) {
    THROW(TokenException::last_error("Failed to generate random signing key"));
  }
  return SigningKey(raw_key);
}

SignedToken SigningKey::sign(BlindedToken tok) const {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_SignedToken> raw_signed(
      signing_key_sign(raw.get(), tok.raw.get()), signed_token_destroy);
  if (raw_signed == nullptr) {
    THROW(TokenException::last_error("Failed to sign blinded token"));
  }

  return SignedToken(raw_signed);
}

UnblindedToken SigningKey::rederive_unblinded_token(TokenPreimage t) {
  CLEAR_LAST_EXCEPTION();
  return UnblindedToken(std::shared_ptr<C_UnblindedToken>(
      signing_key_rederive_unblinded_token(raw.get(), t.raw.get()),
      unblinded_token_destroy));
}

PublicKey SigningKey::public_key() {
  CLEAR_LAST_EXCEPTION();
  return PublicKey(std::shared_ptr<C_PublicKey>(
      signing_key_get_public_key(raw.get()), public_key_destroy));
}

SigningKey SigningKey::decode_base64(const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_SigningKey> raw_key(
      signing_key_decode_base64((const uint8_t*)encoded.data(),
                                encoded.length()),
      signing_key_destroy);
  if (raw_key == nullptr) {
    THROW(TokenException::last_error("Failed to decode signing key"));
  }
  return SigningKey(raw_key);
}

std::string SigningKey::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = signing_key_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(TokenException::last_error("Failed to encode signing key"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}

bool SigningKey::operator==(const SigningKey& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool SigningKey::operator!=(const SigningKey& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto

// class PublicKey
namespace challenge_bypass_ristretto {
PublicKey::PublicKey(std::shared_ptr<C_PublicKey> raw) : raw(raw) {}
PublicKey::PublicKey(const PublicKey& other) = default;
PublicKey::~PublicKey() {}

PublicKey PublicKey::decode_base64(const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_PublicKey> raw_key(
      public_key_decode_base64((const uint8_t*)encoded.data(),
                               encoded.length()),
      public_key_destroy);
  if (raw_key == nullptr) {
    THROW(TokenException::last_error("Failed to decode public key"));
  }
  return PublicKey(raw_key);
}

std::string PublicKey::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = public_key_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(TokenException::last_error("Failed to encode public key"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}

bool PublicKey::operator==(const PublicKey& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool PublicKey::operator!=(const PublicKey& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto

// class DLEQProof
namespace challenge_bypass_ristretto {
DLEQProof::DLEQProof(std::shared_ptr<C_DLEQProof> raw) : raw(raw) {}
DLEQProof::DLEQProof(const DLEQProof& other) = default;
DLEQProof::~DLEQProof() {}

DLEQProof::DLEQProof(BlindedToken blinded_token,
                     SignedToken signed_token,
                     SigningKey key) {
  CLEAR_LAST_EXCEPTION();
  raw = std::shared_ptr<C_DLEQProof>(
      dleq_proof_new(blinded_token.raw.get(), signed_token.raw.get(),
                     key.raw.get()),
      dleq_proof_destroy);
  if (raw == nullptr) {
    THROW(TokenException::last_error("Failed to create new DLEQ proof"));
  }
}

bool DLEQProof::verify(BlindedToken blinded_token,
                       SignedToken signed_token,
                       PublicKey key) {
  CLEAR_LAST_EXCEPTION();
  int result = dleq_proof_invalid(raw.get(), blinded_token.raw.get(),
                                  signed_token.raw.get(), key.raw.get());
  if (result < 0) {
    THROW(TokenException::last_error("Failed to verify DLEQ proof"));
  }
  return result == 0;
}

DLEQProof DLEQProof::decode_base64(const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_DLEQProof> raw_proof(
      dleq_proof_decode_base64((const uint8_t*)encoded.data(),
                               encoded.length()),
      dleq_proof_destroy);
  if (raw_proof == nullptr) {
    THROW(TokenException::last_error("Failed to decode DLEQ proof"));
  }
  return DLEQProof(raw_proof);
}

std::string DLEQProof::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = dleq_proof_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(TokenException::last_error("Failed to encode DLEQ proof"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}

bool DLEQProof::operator==(const DLEQProof& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool DLEQProof::operator!=(const DLEQProof& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto

// class BatchDLEQProof
namespace challenge_bypass_ristretto {
BatchDLEQProof::BatchDLEQProof(std::shared_ptr<C_BatchDLEQProof> raw)
    : raw(raw) {}
BatchDLEQProof::BatchDLEQProof(const BatchDLEQProof& other) = default;
BatchDLEQProof::~BatchDLEQProof() {}

BatchDLEQProof::BatchDLEQProof(std::vector<BlindedToken> blinded_tokens,
                               std::vector<SignedToken> signed_tokens,
                               SigningKey key) {
  CLEAR_LAST_EXCEPTION();
  if (blinded_tokens.size() != signed_tokens.size()) {
    THROW(TokenException(
        "Blinded tokens and signed tokens must have the same length"));
    return;
  }
  std::vector<C_BlindedToken*> raw_blinded_tokens;
  std::vector<C_SignedToken*> raw_signed_tokens;

  for (unsigned long i = 0; i < blinded_tokens.size(); i++) {
    raw_blinded_tokens.push_back(blinded_tokens[i].raw.get());
    raw_signed_tokens.push_back(signed_tokens[i].raw.get());
  }

  raw = std::shared_ptr<C_BatchDLEQProof>(
      batch_dleq_proof_new(raw_blinded_tokens.data(), raw_signed_tokens.data(),
                           blinded_tokens.size(), key.raw.get()),
      batch_dleq_proof_destroy);
  if (raw == nullptr) {
    THROW(TokenException::last_error("Failed to create new batch DLEQ proof"));
  }
}

bool BatchDLEQProof::verify(std::vector<BlindedToken> blinded_tokens,
                            std::vector<SignedToken> signed_tokens,
                            PublicKey key) {
  CLEAR_LAST_EXCEPTION();
  if (blinded_tokens.size() != signed_tokens.size()) {
    THROW(TokenException(
        "Blinded tokens and signed tokens must have the same length"));
    return false;
  }
  std::vector<C_BlindedToken*> raw_blinded_tokens;
  std::vector<C_SignedToken*> raw_signed_tokens;

  for (unsigned long i = 0; i < blinded_tokens.size(); i++) {
    raw_blinded_tokens.push_back(blinded_tokens[i].raw.get());
    raw_signed_tokens.push_back(signed_tokens[i].raw.get());
  }

  int result = batch_dleq_proof_invalid(raw.get(), raw_blinded_tokens.data(),
                                        raw_signed_tokens.data(),
                                        blinded_tokens.size(), key.raw.get());
  if (result < 0) {
    THROW(TokenException::last_error("Could not verify DLEQ proof"));
  }
  return result == 0;
}

std::vector<UnblindedToken> BatchDLEQProof::verify_and_unblind(
    std::vector<Token> tokens,
    std::vector<BlindedToken> blinded_tokens,
    std::vector<SignedToken> signed_tokens,
    PublicKey public_key) {
  CLEAR_LAST_EXCEPTION();

  std::vector<UnblindedToken> unblinded_tokens;

  if (tokens.size() != blinded_tokens.size() ||
      tokens.size() != signed_tokens.size()) {
    THROW(TokenException(
        "Tokens, blinded tokens and signed tokens must have the same length"));
    return unblinded_tokens;
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
    THROW(
        TokenException("Raw tokens, raw blinded tokens and raw signed tokens "
                       "must have the same length"));
    return unblinded_tokens;
  }

  if (!public_key.raw.get()) {
    THROW(TokenException::last_error("Could not verify DLEQ proof"));
    return unblinded_tokens;
  }

  int result = batch_dleq_proof_invalid_or_unblind(
      raw.get(), raw_tokens.data(), raw_blinded_tokens.data(),
      raw_signed_tokens.data(), raw_unblinded_tokens.data(), tokens.size(),
      public_key.raw.get());
  if (result != 0) {
    if (result < 0) {
      THROW(TokenException::last_error("Could not verify DLEQ proof"));
    }
    return unblinded_tokens;
  }

  for (size_t i = 0; i < tokens.size(); i++) {
    std::shared_ptr<C_UnblindedToken> raw_unblinded(raw_unblinded_tokens.at(i),
                                                    unblinded_token_destroy);

    if (raw_unblinded == nullptr) {
      THROW(TokenException::last_error("Unexpected failure to unblind"));
      return unblinded_tokens;
    }

    unblinded_tokens.push_back(UnblindedToken(raw_unblinded));
  }

  return unblinded_tokens;
}

BatchDLEQProof BatchDLEQProof::decode_base64(const std::string encoded) {
  CLEAR_LAST_EXCEPTION();
  std::shared_ptr<C_BatchDLEQProof> raw_proof(
      batch_dleq_proof_decode_base64((const uint8_t*)encoded.data(),
                                     encoded.length()),
      batch_dleq_proof_destroy);
  if (raw_proof == nullptr) {
    THROW(TokenException::last_error("Failed to decode batch DLEQ proof"));
  }
  return BatchDLEQProof(raw_proof);
}

std::string BatchDLEQProof::encode_base64() const {
  CLEAR_LAST_EXCEPTION();
  char* tmp = batch_dleq_proof_encode_base64(raw.get());
  if (tmp == nullptr) {
    THROW(TokenException::last_error("Failed to encode batch DLEQ proof"));
    return "";
  }
  std::string result = std::string(tmp);
  c_char_destroy(tmp);
  return result;
}

bool BatchDLEQProof::operator==(const BatchDLEQProof& rhs) const {
  return encode_base64() == rhs.encode_base64();
}

bool BatchDLEQProof::operator!=(const BatchDLEQProof& rhs) const {
  return !(*this == rhs);
}
}  // namespace challenge_bypass_ristretto
