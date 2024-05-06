// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_EXTENDED_SPENDING_KEY_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_EXTENDED_SPENDING_KEY_IMPL_H_

#include <memory>

#include "brave/components/brave_wallet/browser/zcash/rust/extended_spending_key.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::orchard {

// Implements Orchard key generation from
// https://zips.z.cash/zip-0032#orchard-child-key-derivation
class ExtendedSpendingKeyImpl : ExtendedSpendingKey {
 public:
  ExtendedSpendingKeyImpl(const ExtendedSpendingKeyImpl&) = delete;
  ExtendedSpendingKeyImpl& operator=(const ExtendedSpendingKeyImpl&) = delete;

  ~ExtendedSpendingKeyImpl() override;

  // Derives hardened key using index and the current key
  std::unique_ptr<ExtendedSpendingKey> DeriveHardenedChild(
      uint32_t index) override;

  // Returns public or internal address that may be used as a recipient address
  // in transactions
  std::optional<std::array<uint8_t, kOrchardRawBytesSize>>
  GetDiversifiedAddress(uint32_t div_index, OrchardAddressKind kind) override;

 private:
  friend class ExtendedSpendingKey;
  explicit ExtendedSpendingKeyImpl(rust::Box<OrchardExtendedSpendingKey> esk);
  // Extended spending key is a root key of an account, all other keys can be
  // derived from esk
  rust::Box<OrchardExtendedSpendingKey> extended_spending_key_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_EXTENDED_SPENDING_KEY_IMPL_H_
