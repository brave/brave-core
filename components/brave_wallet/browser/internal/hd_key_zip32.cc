// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/hd_key_zip32.h"

#include <utility>

#include "base/check.h"

static_assert(BUILDFLAG(ENABLE_ORCHARD));

namespace brave_wallet {

HDKeyZip32::HDKeyZip32(rust::Box<OrchardExtendedSpendingKeyResult> esk)
    : extended_spending_key_(std::move(esk)) {}

HDKeyZip32::~HDKeyZip32() = default;

// static
std::unique_ptr<HDKeyZip32> HDKeyZip32::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  auto mk = generate_orchard_extended_spending_key_from_seed(
      rust::Slice<const uint8_t>{seed.data(), seed.size()});
  if (mk->is_ok()) {
    return std::make_unique<HDKeyZip32>(std::move(mk));
  }
  return nullptr;
}

std::unique_ptr<HDKeyZip32> HDKeyZip32::DeriveHardenedChild(uint32_t index) {
  CHECK(extended_spending_key_->is_ok());

  auto esk = extended_spending_key_->unwrap().derive(index);
  if (esk->is_ok()) {
    return std::make_unique<HDKeyZip32>(std::move(esk));
  }
  return nullptr;
}

std::optional<std::array<uint8_t, kOrchardRawBytesSize>>
HDKeyZip32::GetDiversifiedAddress(uint32_t div_index, OrchardKind kind) {
  CHECK(extended_spending_key_->is_ok());
  return kind == OrchardKind::External
             ? extended_spending_key_->unwrap().external_address(div_index)
             : extended_spending_key_->unwrap().internal_address(div_index);
}

}  // namespace brave_wallet
