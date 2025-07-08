// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLER_BROWSER_POLKADOT_RUST_SR25519_H_
#define BRAVE_COMPONENTS_BRAVE_WALLER_BROWSER_POLKADOT_RUST_SR25519_H_

#include <memory>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/polkadot/rust/lib.rs.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet {

namespace polkadot {}  // namespace polkadot

class HDKeySr25519 {
 public:
  static std::unique_ptr<HDKeySr25519> GenerateFromSeed(
      base::span<const uint8_t> seed);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLER_BROWSER_POLKADOT_RUST_SR25519_H_
