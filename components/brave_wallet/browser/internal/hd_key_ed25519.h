/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/rust/lib.rs.h"

namespace brave_wallet {

// This class implement basic EdDSA over ed25519 functionality of bip32-ed25519
// spec with 32 bytes private key and only allows private key derivation with
// hardened index.
class HDKeyEd25519 {
 public:
  HDKeyEd25519(std::string path,
               rust::Box<Ed25519DalekExtendedSecretKeyResult>);
  ~HDKeyEd25519();
  HDKeyEd25519(const HDKeyEd25519&) = delete;
  HDKeyEd25519& operator=(const HDKeyEd25519&) = delete;

  static std::unique_ptr<HDKeyEd25519> GenerateFromSeed(
      base::span<const uint8_t> seed);
  static std::unique_ptr<HDKeyEd25519> GenerateFromPrivateKey(
      base::span<const uint8_t> private_key);

  std::string GetPath() const;
  std::unique_ptr<HDKeyEd25519> DeriveHardenedChild(uint32_t index);

  // If path contains normal index, nullptr will be returned
  std::unique_ptr<HDKeyEd25519> DeriveChildFromPath(const std::string& path);
  std::vector<uint8_t> Sign(base::span<const uint8_t> msg);
  bool VerifyForTesting(base::span<const uint8_t> msg,
                        base::span<const uint8_t> sig);

  std::vector<uint8_t> GetPrivateKeyBytes() const;
  std::vector<uint8_t> GetPublicKeyBytes() const;

  std::string GetBase58EncodedPublicKey() const;
  std::string GetBase58EncodedKeypair() const;

 private:
  std::string path_;
  rust::Box<Ed25519DalekExtendedSecretKeyResult> private_key_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_
