// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_EXTENDED_SPENDING_KEY_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_EXTENDED_SPENDING_KEY_IMPL_H_

#include <memory>

#include "base/types/pass_key.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_extended_spending_key.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::orchard {

// Implements Orchard key generation from
// https://zips.z.cash/zip-0032#orchard-child-key-derivation
class OrchardExtendedSpendingKeyImpl : public OrchardExtendedSpendingKey {
 public:
  OrchardExtendedSpendingKeyImpl(
      absl::variant<base::PassKey<class OrchardExtendedSpendingKey>,
                    base::PassKey<class OrchardExtendedSpendingKeyImpl>>,
      rust::Box<CxxOrchardExtendedSpendingKey>);
  OrchardExtendedSpendingKeyImpl(const OrchardExtendedSpendingKeyImpl&) =
      delete;
  OrchardExtendedSpendingKeyImpl& operator=(
      const OrchardExtendedSpendingKeyImpl&) = delete;

  ~OrchardExtendedSpendingKeyImpl() override;

  // Derives hardened key using index and the current key
  std::unique_ptr<OrchardExtendedSpendingKey> DeriveHardenedChild(
      uint32_t index) override;

  // Returns public or internal address that may be used as a recipient address
  // in transactions
  std::optional<OrchardAddrRawPart> GetDiversifiedAddress(
      uint32_t div_index,
      OrchardAddressKind kind) override;

  OrchardFullViewKey GetFullViewKey() override;

  OrchardSpendingKey GetSpendingKey() override;

 private:
  // Extended spending key is a root key of an account, all other keys can be
  // derived from esk
  rust::Box<CxxOrchardExtendedSpendingKey> cxx_extended_spending_key_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_EXTENDED_SPENDING_KEY_IMPL_H_
