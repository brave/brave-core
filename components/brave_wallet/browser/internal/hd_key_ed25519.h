/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_

#include <memory>
#include <string>

#include "base/containers/span.h"

namespace brave_wallet {

constexpr size_t kEd25519SecretKeySize = 32;
constexpr size_t kEd25519PublicKeySize = 32;
constexpr size_t kEd25519KeypairSize =
    kEd25519SecretKeySize + kEd25519PublicKeySize;
constexpr size_t kEd25519ChainCodeSize = 32;
constexpr size_t kEd25519SignatureSize = 64;

// This class implements basic EdDSA over ed25519 functionality of SLIP-0010
// spec with 32 bytes private key and only allows private key derivation with
// hardened index.
// https://github.com/satoshilabs/slips/blob/master/slip-0010.md
class HDKeyEd25519 {
 public:
  HDKeyEd25519();
  ~HDKeyEd25519();
  HDKeyEd25519(const HDKeyEd25519&) = delete;
  HDKeyEd25519& operator=(const HDKeyEd25519&) = delete;

  static std::unique_ptr<HDKeyEd25519> GenerateFromSeedAndPath(
      base::span<const uint8_t> seed,
      std::string_view hd_path);
  static std::unique_ptr<HDKeyEd25519> GenerateFromKeyPair(
      base::span<const uint8_t, kEd25519KeypairSize> key_pair);

  std::unique_ptr<HDKeyEd25519> DeriveHardenedChild(uint32_t index);

  std::array<uint8_t, kEd25519SignatureSize> Sign(
      base::span<const uint8_t> msg);

  base::span<const uint8_t, kEd25519SecretKeySize> GetPrivateKeyAsSpan() const
      LIFETIME_BOUND;
  base::span<const uint8_t, kEd25519PublicKeySize> GetPublicKeyAsSpan() const
      LIFETIME_BOUND;

  std::string GetBase58EncodedPublicKey() const;
  std::string GetBase58EncodedKeypair() const;

 private:
  HDKeyEd25519(base::span<const uint8_t> key, base::span<const uint8_t> data);

  std::unique_ptr<HDKeyEd25519> DeriveChild(uint32_t index);

  std::array<uint8_t, kEd25519KeypairSize> key_pair_ = {};
  std::array<uint8_t, kEd25519ChainCodeSize> chain_code_ = {};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_
