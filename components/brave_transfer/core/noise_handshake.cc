// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/noise_handshake.h"

#include <array>
#include <limits>
#include <string_view>
#include <utility>

#include "base/containers/extend.h"
#include "base/no_destructor.h"
#include "base/numerics/byte_conversions.h"
#include "base/types/expected_macros.h"
#include "crypto/aead.h"
#include "crypto/hash.h"
#include "crypto/kdf.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_transfer {

namespace {

constexpr size_t kDhSize = 32;
constexpr size_t kHashSize = 32;

// Noise HKDF(chaining_key, ikm) with two or three outputs is RFC 5869 HKDF
// with salt = chaining_key and empty info.
template <size_t N>
std::array<Key, N> NoiseHkdf(const Key& chaining_key,
                             base::span<const uint8_t> ikm) {
  const auto out = crypto::kdf::Hkdf<N * kKeySize>(
      crypto::hash::kSha256, ikm, chaining_key, base::span<const uint8_t>());
  std::array<Key, N> keys;
  for (size_t i = 0; i < N; ++i) {
    base::span(keys[i]).copy_from(
        base::span(out).subspan(i * kKeySize).template first<kKeySize>());
  }
  return keys;
}

Key ToKey(base::span<const uint8_t> bytes) {
  Key key;
  base::span(key).copy_from(bytes.first<kKeySize>());
  return key;
}

}  // namespace

CipherState::CipherState() = default;
CipherState::CipherState(const Key& key) : key_(key) {}
CipherState::CipherState(const CipherState&) = default;
CipherState& CipherState::operator=(const CipherState&) = default;
CipherState::~CipherState() = default;

std::optional<std::array<uint8_t, 12>> CipherState::NextNonce() {
  if (nonce_ == std::numeric_limits<uint64_t>::max()) {
    return std::nullopt;
  }
  std::array<uint8_t, 12> nonce = {};
  base::span(nonce).last<8>().copy_from(base::U64ToLittleEndian(nonce_));
  ++nonce_;
  return nonce;
}

Result<Bytes> CipherState::Encrypt(base::span<const uint8_t> ad,
                                   base::span<const uint8_t> plaintext) {
  if (!key_) {
    return Bytes(plaintext.begin(), plaintext.end());
  }
  const auto nonce = NextNonce();
  if (!nonce) {
    return base::unexpected("nonce exhausted");
  }
  return crypto::aead::Seal(crypto::aead::CHACHA20_POLY1305, *key_, plaintext,
                            *nonce, ad);
}

Result<Bytes> CipherState::Decrypt(base::span<const uint8_t> ad,
                                   base::span<const uint8_t> ciphertext) {
  if (!key_) {
    return Bytes(ciphertext.begin(), ciphertext.end());
  }
  const auto nonce = NextNonce();
  if (!nonce) {
    return base::unexpected("nonce exhausted");
  }
  auto plaintext = crypto::aead::Open(crypto::aead::CHACHA20_POLY1305, *key_,
                                      ciphertext, *nonce, ad);
  if (!plaintext) {
    return base::unexpected("decryption failed");
  }
  return std::move(*plaintext);
}

NoiseHandshake::NoiseHandshake(bool initiator,
                               base::span<const uint8_t> prologue,
                               const Key& static_private_key,
                               std::optional<Key> psk,
                               std::optional<Key> ephemeral_private_key)
    : initiator_(initiator),
      static_private_key_(static_private_key),
      static_public_key_(static_key::PublicKey(static_private_key)),
      ephemeral_private_key_(std::move(ephemeral_private_key)),
      psk_(std::move(psk)) {
  constexpr std::string_view kName = kNoiseProtocolName;
  static_assert(kName.size() > kHashSize);
  h_ = crypto::hash::Sha256(base::as_byte_span(kName));
  chaining_key_ = h_;
  MixHash(prologue);
}

NoiseHandshake::~NoiseHandshake() = default;

bool NoiseHandshake::IsComplete() const {
  return message_index_ == 3;
}

bool NoiseHandshake::IsMyTurn() const {
  return (message_index_ % 2 == 0) == initiator_;
}

Result<Key> NoiseHandshake::Dh(Token token) const {
  if (!ephemeral_private_key_ || !remote_ephemeral_key_) {
    return base::unexpected("missing ephemeral key");
  }
  const Key* private_key = nullptr;
  const Key* public_key = nullptr;
  switch (token) {
    case Token::kEE:
      private_key = &*ephemeral_private_key_;
      public_key = &*remote_ephemeral_key_;
      break;
    case Token::kES:
      if (initiator_) {
        if (!remote_static_key_) {
          return base::unexpected("missing remote static key");
        }
        private_key = &*ephemeral_private_key_;
        public_key = &*remote_static_key_;
      } else {
        private_key = &static_private_key_;
        public_key = &*remote_ephemeral_key_;
      }
      break;
    case Token::kSE:
      if (initiator_) {
        private_key = &static_private_key_;
        public_key = &*remote_ephemeral_key_;
      } else {
        if (!remote_static_key_) {
          return base::unexpected("missing remote static key");
        }
        private_key = &*ephemeral_private_key_;
        public_key = &*remote_static_key_;
      }
      break;
    default:
      return base::unexpected("not a DH token");
  }
  Key shared;
  // X25519() returns 0 when the result is all zeros, i.e. the peer sent a
  // low-order point. PROTOCOL.md §4.2 requires aborting in that case.
  if (!X25519(shared.data(), private_key->data(), public_key->data())) {
    return base::unexpected("low-order public key");
  }
  return shared;
}

void NoiseHandshake::MixKey(base::span<const uint8_t> ikm) {
  const auto [ck, temp_k] = NoiseHkdf<2>(chaining_key_, ikm);
  chaining_key_ = ck;
  cipher_ = CipherState(temp_k);
}

void NoiseHandshake::MixHash(base::span<const uint8_t> data) {
  crypto::hash::Hasher hasher(crypto::hash::kSha256);
  hasher.Update(h_);
  hasher.Update(data);
  hasher.Finish(h_);
}

void NoiseHandshake::MixKeyAndHash(base::span<const uint8_t> ikm) {
  const auto [ck, temp_h, temp_k] = NoiseHkdf<3>(chaining_key_, ikm);
  chaining_key_ = ck;
  MixHash(temp_h);
  cipher_ = CipherState(temp_k);
}

Result<Bytes> NoiseHandshake::EncryptAndHash(
    base::span<const uint8_t> plaintext) {
  ASSIGN_OR_RETURN(Bytes ciphertext, cipher_.Encrypt(h_, plaintext));
  MixHash(ciphertext);
  return ciphertext;
}

Result<Bytes> NoiseHandshake::DecryptAndHash(
    base::span<const uint8_t> ciphertext) {
  ASSIGN_OR_RETURN(Bytes plaintext, cipher_.Decrypt(h_, ciphertext));
  MixHash(ciphertext);
  return plaintext;
}

// static
const std::array<std::vector<NoiseHandshake::Token>, 3>&
NoiseHandshake::Pattern() {
  static const base::NoDestructor<std::array<std::vector<Token>, 3>> pattern(
      std::array<std::vector<Token>, 3>{{
          {Token::kE},
          {Token::kE, Token::kEE, Token::kS, Token::kES},
          {Token::kS, Token::kSE, Token::kPsk},
      }});
  return *pattern;
}

Result<Bytes> NoiseHandshake::WriteMessage(base::span<const uint8_t> payload) {
  if (IsComplete() || !IsMyTurn()) {
    return base::unexpected("not our turn to write");
  }
  Bytes out;
  for (Token token : Pattern()[message_index_]) {
    switch (token) {
      case Token::kE: {
        if (!ephemeral_private_key_) {
          ephemeral_private_key_ = static_key::GeneratePrivateKey();
        }
        const Key e_pub = static_key::PublicKey(*ephemeral_private_key_);
        base::Extend(out, e_pub);
        MixHash(e_pub);
        // Required by the psk modifier.
        MixKey(e_pub);
        break;
      }
      case Token::kS: {
        ASSIGN_OR_RETURN(Bytes encrypted, EncryptAndHash(static_public_key_));
        base::Extend(out, encrypted);
        break;
      }
      case Token::kPsk:
        if (!psk_) {
          return base::unexpected("psk not set");
        }
        MixKeyAndHash(*psk_);
        break;
      default: {
        ASSIGN_OR_RETURN(Key shared, Dh(token));
        MixKey(shared);
        break;
      }
    }
  }
  ASSIGN_OR_RETURN(Bytes encrypted_payload, EncryptAndHash(payload));
  base::Extend(out, encrypted_payload);
  if (out.size() > kMaxNoiseMessage) {
    return base::unexpected("message too long");
  }
  Advance();
  return out;
}

Result<Bytes> NoiseHandshake::ReadMessage(base::span<const uint8_t> message) {
  if (IsComplete() || IsMyTurn()) {
    return base::unexpected("not our turn to read");
  }
  if (message.size() > kMaxNoiseMessage) {
    return base::unexpected("message too long");
  }
  base::span<const uint8_t> rest = message;
  for (Token token : Pattern()[message_index_]) {
    switch (token) {
      case Token::kE: {
        if (rest.size() < kDhSize) {
          return base::unexpected("truncated message");
        }
        const Key re = ToKey(rest.take_first<kDhSize>());
        remote_ephemeral_key_ = re;
        MixHash(re);
        MixKey(re);
        break;
      }
      case Token::kS: {
        const size_t n = kDhSize + (cipher_.has_key() ? kNoiseTagSize : 0);
        if (rest.size() < n) {
          return base::unexpected("truncated message");
        }
        ASSIGN_OR_RETURN(Bytes rs, DecryptAndHash(rest.take_first(n)));
        remote_static_key_ = ToKey(rs);
        break;
      }
      case Token::kPsk:
        if (!psk_) {
          return base::unexpected("psk not set");
        }
        MixKeyAndHash(*psk_);
        break;
      default: {
        ASSIGN_OR_RETURN(Key shared, Dh(token));
        MixKey(shared);
        break;
      }
    }
  }
  if (cipher_.has_key() && rest.size() < kNoiseTagSize) {
    return base::unexpected("truncated message");
  }
  ASSIGN_OR_RETURN(Bytes payload, DecryptAndHash(rest));
  Advance();
  return payload;
}

void NoiseHandshake::Advance() {
  ++message_index_;
  if (!IsComplete()) {
    return;
  }
  const auto [k1, k2] =
      NoiseHkdf<2>(chaining_key_, base::span<const uint8_t>());
  if (initiator_) {
    send_cipher_.emplace(k1);
    receive_cipher_.emplace(k2);
  } else {
    send_cipher_.emplace(k2);
    receive_cipher_.emplace(k1);
  }
  handshake_hash_ = h_;
}

}  // namespace brave_transfer
