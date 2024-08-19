// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/extended_spending_key_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"

namespace brave_wallet::orchard {

ExtendedSpendingKeyImpl::ExtendedSpendingKeyImpl(
    rust::Box<OrchardExtendedSpendingKey> esk)
    : extended_spending_key_(std::move(esk)) {}

ExtendedSpendingKeyImpl::~ExtendedSpendingKeyImpl() = default;

std::unique_ptr<ExtendedSpendingKey>
ExtendedSpendingKeyImpl::DeriveHardenedChild(uint32_t index) {
  auto esk = extended_spending_key_->derive(index);
  if (esk->is_ok()) {
    return base::WrapUnique<ExtendedSpendingKey>(
        new ExtendedSpendingKeyImpl(esk->unwrap()));
  }
  return nullptr;
}

std::optional<std::array<uint8_t, kOrchardRawBytesSize>>
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
    return base::WrapUnique<ExtendedSpendingKey>(
        new ExtendedSpendingKeyImpl(mk->unwrap()));
  }
  return nullptr;
}

OrchardFullViewKey ExtendedSpendingKeyImpl::GetFullViewKey() {
  return extended_spending_key_->full_view_key();
}

}  // namespace brave_wallet::orchard
