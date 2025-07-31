// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_SR25519_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_SR25519_H_

#include <array>

#include "base/containers/span.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet {

struct CxxSchnorrkelKeyPair;

inline constexpr size_t kSr25519SeedSize = 32;
inline constexpr size_t kSr25519PublicKeySize = 32;
inline constexpr size_t kSr25519SignatureSize = 64;

class HDKeySr25519 {
 public:
  HDKeySr25519(const HDKeySr25519&) = delete;
  HDKeySr25519& operator=(const HDKeySr25519&) = delete;

  HDKeySr25519(HDKeySr25519&&) noexcept;
  HDKeySr25519& operator=(HDKeySr25519&&) noexcept;

  ~HDKeySr25519();

  static HDKeySr25519 GenerateFromSeed(
      base::span<const uint8_t, kSr25519SeedSize> seed);

  std::array<uint8_t, kSr25519PublicKeySize> GetPublicKey();
  std::array<uint8_t, kSr25519SignatureSize> SignMessage(
      base::span<const uint8_t> msg);
  [[nodiscard]] bool VerifyMessage(
      std::array<uint8_t, kSr25519SignatureSize> const& sig,
      base::span<const uint8_t> msg);

  // derive_junction should be a SCALE-encoded segment from the derivation path
  HDKeySr25519 DeriveHard(base::span<const uint8_t> derive_junction);

 private:
  explicit HDKeySr25519(rust::Box<CxxSchnorrkelKeyPair> keypair);

  rust::Box<CxxSchnorrkelKeyPair> keypair_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_SR25519_H_
