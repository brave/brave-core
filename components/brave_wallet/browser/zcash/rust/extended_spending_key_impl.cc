// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/extended_spending_key_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"

namespace brave_wallet::orchard {

ExtendedSpendingKeyImpl::ExtendedSpendingKeyImpl(
    absl::variant<base::PassKey<class ExtendedSpendingKey>,
                  base::PassKey<class ExtendedSpendingKeyImpl>>,
    rust::Box<CxxOrchardExtendedSpendingKey> esk)
    : extended_spending_key_(std::move(esk)) {}

ExtendedSpendingKeyImpl::~ExtendedSpendingKeyImpl() = default;

std::unique_ptr<ExtendedSpendingKey>
ExtendedSpendingKeyImpl::DeriveHardenedChild(uint32_t index) {
  auto esk = extended_spending_key_->derive(index);
  if (esk->is_ok()) {
    return std::make_unique<ExtendedSpendingKeyImpl>(
        base::PassKey<class ExtendedSpendingKeyImpl>(), esk->unwrap());
  }
  return nullptr;
}

std::optional<OrchardAddrRawPart>
ExtendedSpendingKeyImpl::GetDiversifiedAddress(uint32_t div_index,
                                               OrchardAddressKind kind) {
  return kind == OrchardAddressKind::External
             ? extended_spending_key_->external_address(div_index)
             : extended_spending_key_->internal_address(div_index);
}

// static
std::unique_ptr<ExtendedSpendingKey> ExtendedSpendingKey::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  auto mk = generate_orchard_extended_spending_key_from_seed(
      rust::Slice<const uint8_t>{seed.data(), seed.size()});
  if (mk->is_ok()) {
    return std::make_unique<ExtendedSpendingKeyImpl>(
        base::PassKey<class ExtendedSpendingKey>(), mk->unwrap());
  }
  return nullptr;
}

OrchardFullViewKey ExtendedSpendingKeyImpl::GetFullViewKey() {
  return extended_spending_key_->full_view_key();
}

OrchardSpendingKey ExtendedSpendingKeyImpl::GetSpendingKey() {
  return extended_spending_key_->spending_key();
}

}  // namespace brave_wallet::orchard
