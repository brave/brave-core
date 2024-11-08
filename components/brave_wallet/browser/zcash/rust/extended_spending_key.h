// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_EXTENDED_SPENDING_KEY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_EXTENDED_SPENDING_KEY_H_

#include <memory>
#include <optional>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

// Implements Orchard key generation from
// https://zips.z.cash/zip-0032#orchard-child-key-derivation
class ExtendedSpendingKey {
 public:
  virtual ~ExtendedSpendingKey() = default;

  // Generates master key using provided seed
  static std::unique_ptr<ExtendedSpendingKey> GenerateFromSeed(
      base::span<const uint8_t> seed);

  // Derives hardened key using index and the current key
  virtual std::unique_ptr<ExtendedSpendingKey> DeriveHardenedChild(
      uint32_t index) = 0;

  // Returns public or internal address that may be used as a recipient address
  // in transactions
  virtual std::optional<OrchardAddrRawPart> GetDiversifiedAddress(
      uint32_t div_index,
      OrchardAddressKind kind) = 0;

  virtual OrchardSpendingKey GetSpendingKey() = 0;

  virtual OrchardFullViewKey GetFullViewKey() = 0;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_EXTENDED_SPENDING_KEY_H_
