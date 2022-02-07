/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/internal/hd_key_base.h"
#include "brave/components/brave_wallet/rust/lib.rs.h"

namespace brave_wallet {

// This class implement basic functionality of bip32-ed25519 spec
// with 32 bytes private key and only allows private key derivation with
// hardened index
class HDKeyEd25519 : public HDKeyBase {
 public:
  explicit HDKeyEd25519(rust::Box<Ed25519DalekExtendedSecretKeyResult>);
  ~HDKeyEd25519() override;
  HDKeyEd25519(const HDKeyEd25519&) = delete;
  HDKeyEd25519& operator=(const HDKeyEd25519&) = delete;

  static std::unique_ptr<HDKeyEd25519> GenerateFromSeed(
      const std::vector<uint8_t>& seed);
  static std::unique_ptr<HDKeyEd25519> GenerateFromPrivateKey(
      const std::vector<uint8_t>& private_key);

  // Always performs harden derivation
  // index will automatically transformed to hardened index
  // if index >= 2^31, nullptr will be returned
  std::unique_ptr<HDKeyBase> DeriveChild(uint32_t index) override;
  // If path contains normal index, nullptr will be returned
  std::unique_ptr<HDKeyBase> DeriveChildFromPath(
      const std::string& path) override;
  std::vector<uint8_t> Sign(const std::vector<uint8_t>& msg,
                            int* recid = nullptr) override;
  bool Verify(const std::vector<uint8_t>& msg,
              const std::vector<uint8_t>& sig) override;

  std::string GetEncodedPrivateKey() const override;

  std::string GetBase58EncodedPublicKey() const;
  std::string GetBase58EncodedKeypair() const;

 private:
  rust::Box<Ed25519DalekExtendedSecretKeyResult> private_key_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_
