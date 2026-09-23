// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ZIP32_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ZIP32_H_

#include <memory>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

namespace orchard {
class OrchardExtendedSpendingKey;
}  // namespace orchard

// Implements Orchard key generation from
// https://zips.z.cash/zip-0032#orchard-child-key-derivation
class HDKeyZip32 {
 public:
  HDKeyZip32(const HDKeyZip32&) = delete;
  HDKeyZip32& operator=(const HDKeyZip32&) = delete;

  ~HDKeyZip32();

  // Generates master key using provided seed
  static std::unique_ptr<HDKeyZip32> GenerateFromSeed(
      base::span<const uint8_t> seed);

  // Derives hardened key using index and the current key
  std::unique_ptr<HDKeyZip32> DeriveHardenedChild(uint32_t index);

  // Returns public or internal address that may be used as a recipient address
  // in transactions
  std::optional<OrchardAddrRawPart> GetDiversifiedAddress(
      uint32_t div_index,
      OrchardAddressKind kind);

  // Full view key(fvk) is used to decode incoming transactions
  OrchardFullViewKey GetFullViewKey();

  OrchardSpendingKey GetSpendingKey();

 private:
  explicit HDKeyZip32(std::unique_ptr<orchard::OrchardExtendedSpendingKey> key);
  // Extended spending key is a root key of an account, all other keys can be
  // derived from esk
  std::unique_ptr<orchard::OrchardExtendedSpendingKey>
      orchard_extended_spending_key_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ZIP32_H_
