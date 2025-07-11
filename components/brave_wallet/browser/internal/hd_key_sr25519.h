// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_SR25519_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_SR25519_H_

#include <memory>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/polkadot/rust/sr25519.h"

namespace brave_wallet {

namespace polkadot {
class SchnorrkelKeyPair;
}  // namespace polkadot

using polkadot::SchnorrkelKeyPair;
using polkadot::SR25519PublicKey;
using polkadot::SR25519Signature;

class HDKeySr25519 {
 public:
  HDKeySr25519(const HDKeySr25519&) = delete;
  HDKeySr25519& operator=(const HDKeySr25519&) = delete;

  ~HDKeySr25519();

  static std::unique_ptr<HDKeySr25519> GenerateFromSeed(
      base::span<const uint8_t> seed);

  SR25519PublicKey GetPublicKey();
  SR25519Signature SignMessage(base::span<const uint8_t> msg);
  bool VerifyMessage(SR25519Signature const& sig,
                     base::span<const uint8_t> msg);

 private:
  explicit HDKeySr25519(std::unique_ptr<SchnorrkelKeyPair> key);
  std::unique_ptr<SchnorrkelKeyPair> schnorrkel_key_pair_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_SR25519_H_
