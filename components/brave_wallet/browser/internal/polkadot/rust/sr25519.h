// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_POLKADOT_RUST_SR25519_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_POLKADOT_RUST_SR25519_H_

#include <array>
#include <optional>

#include "base/containers/span.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::polkadot {

using SR25519PublicKey = std::array<uint8_t, 32>;
using SR25519Signature = std::array<uint8_t, 64>;

struct CxxSchnorrkelKeyPair;

class SchnorrkelKeyPair {
 public:
  ~SchnorrkelKeyPair();
  SchnorrkelKeyPair(SchnorrkelKeyPair&&);

  static std::optional<SchnorrkelKeyPair> GenerateFromSeed(
      base::span<const uint8_t> seed);

  SR25519PublicKey GetPublicKey();
  SR25519Signature SignMessage(base::span<const uint8_t> msg);
  bool VerifyMessage(SR25519Signature const& sig,
                     base::span<const uint8_t> msg);
  SchnorrkelKeyPair DeriveHard(base::span<const uint8_t> derive_junction);

 private:
  explicit SchnorrkelKeyPair(rust::Box<CxxSchnorrkelKeyPair> impl);

  rust::Box<CxxSchnorrkelKeyPair> impl_;
};

}  // namespace brave_wallet::polkadot

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_POLKADOT_RUST_SR25519_H_
