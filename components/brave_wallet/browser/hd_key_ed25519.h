/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEY_ED25519_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEY_ED25519_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/rust/lib.rs.h"

namespace brave_wallet {

// This class implement basic functionality of bip32-ed25519 spec
// with 32 bytes private key and only allows private key derivation with
// hardened index
class HDKeyEd25519 {
 public:
  explicit HDKeyEd25519(rust::Box<Ed25519DalekExtendedSecretKey>);
  ~HDKeyEd25519();
  HDKeyEd25519(const HDKeyEd25519&) = delete;
  HDKeyEd25519& operator=(const HDKeyEd25519&) = delete;

  static std::unique_ptr<HDKeyEd25519> GenerateFromSeed(
      const std::vector<uint8_t>& seed);

  // Always performs harden derivation
  // index will automatically transformed to hardened index
  // if index >= 2^31, nullptr will be returned
  std::unique_ptr<HDKeyEd25519> DeriveChild(uint32_t index);
  // path format: m/[n|n']*/[n|n']*...
  // n: 0 to 2^31-1 (normal derivation)
  // n': n + 2^31 (harden derivation)
  // If path contains normal index, nullptr will be returned
  std::unique_ptr<HDKeyEd25519> DeriveChildFromPath(const std::string& path);
  std::string GetBase58EncodedPublicKey() const;
  std::string GetBase58EncodedKeypair() const;

 private:
  rust::Box<Ed25519DalekExtendedSecretKey> private_key_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEY_ED25519_H_
