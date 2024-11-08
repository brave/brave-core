// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/hd_key_zip32.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "brave/components/brave_wallet/browser/zcash/rust/extended_spending_key.h"

namespace brave_wallet {

HDKeyZip32::HDKeyZip32(std::unique_ptr<orchard::ExtendedSpendingKey> esk)
    : extended_spending_key_(std::move(esk)) {}

HDKeyZip32::~HDKeyZip32() = default;

// static
std::unique_ptr<HDKeyZip32> HDKeyZip32::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  return base::WrapUnique(
      new HDKeyZip32(orchard::ExtendedSpendingKey::GenerateFromSeed(seed)));
}

std::unique_ptr<HDKeyZip32> HDKeyZip32::DeriveHardenedChild(uint32_t index) {
  return base::WrapUnique(
      new HDKeyZip32(extended_spending_key_->DeriveHardenedChild(index)));
}

std::optional<OrchardAddrRawPart> HDKeyZip32::GetDiversifiedAddress(
    uint32_t div_index,
    OrchardAddressKind kind) {
  return extended_spending_key_->GetDiversifiedAddress(div_index, kind);
}

OrchardFullViewKey HDKeyZip32::GetFullViewKey() {
  return extended_spending_key_->GetFullViewKey();
}

OrchardSpendingKey HDKeyZip32::GetSpendingKey() {
  return extended_spending_key_->GetSpendingKey();
}

}  // namespace brave_wallet
