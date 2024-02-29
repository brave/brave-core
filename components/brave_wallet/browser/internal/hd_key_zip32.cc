// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/hd_key_zip32.h"

#include <utility>

#include "base/check.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/buildflags.h"

namespace brave_wallet {

HDKeyZip32::HDKeyZip32(rust::Box<OrchardExtendedSpendingKeyResult> esk,
                       const std::string& path)
    : extended_spending_key_(std::move(esk)), path_(path) {}

HDKeyZip32::~HDKeyZip32() {}

// static
std::unique_ptr<HDKeyZip32> HDKeyZip32::GenerateFromSeed(
    const std::vector<uint8_t>& seed) {
#if BUILDFLAG(ENABLE_ORCHARD)
  auto mk = generate_orchard_extended_spending_key_from_seed(
      rust::Slice<const uint8_t>{seed.data(), seed.size()});
  if (mk->is_ok()) {
    return std::make_unique<HDKeyZip32>(std::move(mk), "m'/");
  }
  return nullptr;
#else
  NOTREACHED();
  return nullptr;
#endif
}

std::unique_ptr<HDKeyZip32> HDKeyZip32::DeriveHardenedChild(uint32_t index) {
#if BUILDFLAG(ENABLE_ORCHARD)
  CHECK(extended_spending_key_->is_ok());

  auto esk = extended_spending_key_->unwrap().derive(index);
  if (esk->is_ok()) {
    return std::make_unique<HDKeyZip32>(
        std::move(esk),
        base::StrCat({path_, "/", base::NumberToString(index), "'"}));
  }
  return nullptr;
#else
  NOTREACHED();
  return nullptr;
#endif
}

std::optional<std::vector<uint8_t>> HDKeyZip32::GetPublicDevirsifiedAddress(
    uint32_t div_index,
    OrchardKind kind) {
#if BUILDFLAG(ENABLE_ORCHARD)
  CHECK(extended_spending_key_->is_ok());
  auto bytes =
      kind == OrchardKind::External
          ? extended_spending_key_->unwrap().external_address(div_index)
          : extended_spending_key_->unwrap().internal_address(div_index);
  return std::vector<uint8_t>(bytes.begin(), bytes.end());
#else
  NOTREACHED();
  return std::nullopt;
#endif
}

}  // namespace brave_wallet
