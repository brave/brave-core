/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
  HDKeyEd25519(std::string path,
               rust::Box<Ed25519DalekExtendedSecretKeyResult>);
  ~HDKeyEd25519() override;
  HDKeyEd25519(const HDKeyEd25519&) = delete;
  HDKeyEd25519& operator=(const HDKeyEd25519&) = delete;

  static std::unique_ptr<HDKeyEd25519> GenerateFromSeed(
      const std::vector<uint8_t>& seed);
  static std::unique_ptr<HDKeyEd25519> GenerateFromPrivateKey(
      const std::vector<uint8_t>& private_key);

  std::string GetPath() const override;
  std::unique_ptr<HDKeyBase> DeriveNormalChild(uint32_t index) override;
  std::unique_ptr<HDKeyBase> DeriveHardenedChild(uint32_t index) override;

  // If path contains normal index, nullptr will be returned
  std::unique_ptr<HDKeyBase> DeriveChildFromPath(
      const std::string& path) override;
  std::vector<uint8_t> Sign(const std::vector<uint8_t>& msg);
  bool Verify(const std::vector<uint8_t>& msg,
              const std::vector<uint8_t>& sig) override;

  std::string EncodePrivateKeyForExport() const override;
  std::vector<uint8_t> GetPrivateKeyBytes() const override;
  std::vector<uint8_t> GetPublicKeyBytes() const override;

  std::string GetBase58EncodedPublicKey() const;
  std::string GetBase58EncodedKeypair() const;

 private:
  std::string path_;
  rust::Box<Ed25519DalekExtendedSecretKeyResult> private_key_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_
