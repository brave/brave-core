// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_NOISE_HANDSHAKE_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_NOISE_HANDSHAKE_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/types/expected.h"
#include "brave/components/brave_transfer/core/trust_keys.h"

namespace brave_transfer {

inline constexpr char kNoiseProtocolName[] =
    "Noise_XXpsk3_25519_ChaChaPoly_SHA256";
inline constexpr size_t kNoiseTagSize = 16;
inline constexpr size_t kMaxNoiseMessage = 65535;

template <typename T>
using Result = base::expected<T, std::string>;
using Bytes = std::vector<uint8_t>;

// Noise CipherState with ChaChaPoly. Without a key, Encrypt() and Decrypt()
// pass data through, as the Noise spec requires during the handshake.
class CipherState {
 public:
  CipherState();
  explicit CipherState(const Key& key);
  CipherState(const CipherState&);
  CipherState& operator=(const CipherState&);
  ~CipherState();

  bool has_key() const { return key_.has_value(); }
  const std::optional<Key>& key() const { return key_; }

  Result<Bytes> Encrypt(base::span<const uint8_t> ad,
                        base::span<const uint8_t> plaintext);
  Result<Bytes> Decrypt(base::span<const uint8_t> ad,
                        base::span<const uint8_t> ciphertext);

 private:
  std::optional<std::array<uint8_t, 12>> NextNonce();

  std::optional<Key> key_;
  uint64_t nonce_ = 0;
};

// One side of a Noise_XXpsk3_25519_ChaChaPoly_SHA256 handshake (Noise
// revision 34), exactly the pattern Brave Transfer uses (PROTOCOL.md §4.2):
//
//   -> e
//   <- e, ee, s, es
//   -> s, se, psk
//
// `ephemeral_private_key` is only fixed in tests.
class NoiseHandshake {
 public:
  NoiseHandshake(bool initiator,
                 base::span<const uint8_t> prologue,
                 const Key& static_private_key,
                 std::optional<Key> psk = std::nullopt,
                 std::optional<Key> ephemeral_private_key = std::nullopt);
  NoiseHandshake(const NoiseHandshake&) = delete;
  NoiseHandshake& operator=(const NoiseHandshake&) = delete;
  ~NoiseHandshake();

  bool is_initiator() const { return initiator_; }
  bool IsComplete() const;
  void SetPsk(const Key& psk) { psk_ = psk; }

  Result<Bytes> WriteMessage(base::span<const uint8_t> payload);
  // Returns the decrypted payload.
  Result<Bytes> ReadMessage(base::span<const uint8_t> message);

  const std::optional<Key>& remote_static_key() const {
    return remote_static_key_;
  }
  const std::optional<Key>& remote_ephemeral_key() const {
    return remote_ephemeral_key_;
  }

  // Set once the handshake is complete.
  CipherState* send_cipher() { return send_cipher_ ? &*send_cipher_ : nullptr; }
  CipherState* receive_cipher() {
    return receive_cipher_ ? &*receive_cipher_ : nullptr;
  }
  const std::optional<Key>& handshake_hash() const { return handshake_hash_; }

 private:
  enum class Token { kE, kS, kEE, kES, kSE, kPsk };

  static const std::array<std::vector<Token>, 3>& Pattern();

  bool IsMyTurn() const;
  Result<Key> Dh(Token token) const;
  void MixKey(base::span<const uint8_t> ikm);
  void MixHash(base::span<const uint8_t> data);
  void MixKeyAndHash(base::span<const uint8_t> ikm);
  Result<Bytes> EncryptAndHash(base::span<const uint8_t> plaintext);
  Result<Bytes> DecryptAndHash(base::span<const uint8_t> ciphertext);
  void Advance();

  const bool initiator_;
  const Key static_private_key_;
  const Key static_public_key_;
  std::optional<Key> ephemeral_private_key_;
  std::optional<Key> psk_;
  size_t message_index_ = 0;

  Key chaining_key_;
  Key h_;
  CipherState cipher_;

  std::optional<Key> remote_static_key_;
  std::optional<Key> remote_ephemeral_key_;
  std::optional<CipherState> send_cipher_;
  std::optional<CipherState> receive_cipher_;
  std::optional<Key> handshake_hash_;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_NOISE_HANDSHAKE_H_
