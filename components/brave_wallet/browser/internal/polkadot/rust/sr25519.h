// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLER_BROWSER_POLKADOT_RUST_SR25519_H_
#define BRAVE_COMPONENTS_BRAVE_WALLER_BROWSER_POLKADOT_RUST_SR25519_H_

#include <array>
#include <memory>

#include "base/containers/span.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::schnorrkel {

using SR25519PublicKey = std::array<uint8_t, 32>;

class SchnorrkelKeyPair {
 public:
  virtual ~SchnorrkelKeyPair() = default;

  static std::unique_ptr<SchnorrkelKeyPair> GenerateFromSeed(
      base::span<const uint8_t> seed);

  virtual SR25519PublicKey GetPublicKey() = 0;
};

}  // namespace brave_wallet::schnorrkel

#endif  // BRAVE_COMPONENTS_BRAVE_WALLER_BROWSER_POLKADOT_RUST_SR25519_H_
