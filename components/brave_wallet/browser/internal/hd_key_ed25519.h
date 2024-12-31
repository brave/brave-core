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
#include "base/gtest_prod_util.h"

namespace brave_wallet {

// https://www.rfc-editor.org/rfc/rfc8032.html#section-5.1.5
inline constexpr size_t kEd25519PrivateKeySize = 32;
inline constexpr size_t kEd25519PublicKeySize = 32;
inline constexpr size_t kEd25519KeyPairSize =
    kEd25519PrivateKeySize + kEd25519PublicKeySize;

// https://github.com/satoshilabs/slips/blob/de7f963959ccfc80256fb5e001f64ce9ada9fba1/slip-0010.md?plain=1#L116-L117
inline constexpr size_t kSlip10ChainCodeSize = 32;

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

  static std::unique_ptr<HDKeyEd25519> GenerateFromSeed(
      base::span<const uint8_t> seed);
  static std::unique_ptr<HDKeyEd25519> GenerateFromPrivateKey(
      base::span<const uint8_t> private_key);

  std::unique_ptr<HDKeyEd25519> DeriveHardenedChild(uint32_t index);

  std::vector<uint8_t> Sign(base::span<const uint8_t> msg);

  std::vector<uint8_t> GetPrivateKeyBytes() const;
  std::vector<uint8_t> GetPublicKeyBytes() const;

  std::string GetBase58EncodedPublicKey() const;
  std::string GetBase58EncodedKeypair() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(HDKeyEd25519UnitTest, TestVector1);
  FRIEND_TEST_ALL_PREFIXES(HDKeyEd25519UnitTest, TestVector2);

  base::span<const uint8_t, kEd25519PrivateKeySize> GetPrivateKeyAsSpan() const;
  base::span<const uint8_t, kEd25519PublicKeySize> GetPublicKeyAsSpan() const;

  static std::unique_ptr<HDKeyEd25519> DeriveFromHmacPayload(
      base::span<const uint8_t> key,
      base::span<const uint8_t> data);

  std::array<uint8_t, kEd25519KeyPairSize> key_pair_ = {};
  std::array<uint8_t, kSlip10ChainCodeSize> chain_code_ = {};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ED25519_H_
