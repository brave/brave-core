/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"

#include <cstdint>
#include <optional>

namespace brave_wallet {

DerivationIndex::DerivationIndex(uint32_t index, bool is_hardened)
    : index_(index), is_hardened_(is_hardened) {}

// static
DerivationIndex DerivationIndex::Normal(uint32_t index) {
  return DerivationIndex(index, false);
}

// static
DerivationIndex DerivationIndex::Hardened(uint32_t index) {
  return DerivationIndex(index, true);
}

bool DerivationIndex::IsValid() const {
  return index_ < kHardenedOffset;
}

std::optional<uint32_t> DerivationIndex::GetValue() const {
  if (!IsValid()) {
    return std::nullopt;
  }
  return index_ + (is_hardened_ ? kHardenedOffset : 0);
}

}  // namespace brave_wallet
