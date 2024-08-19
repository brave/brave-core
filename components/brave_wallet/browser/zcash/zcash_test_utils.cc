/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"

#include <algorithm>
#include <vector>

namespace brave_wallet {

std::array<uint8_t, kOrchardNullifierSize> GenerateMockNullifier(
    const mojom::AccountIdPtr& account_id,
    uint8_t seed) {
  std::array<uint8_t, kOrchardNullifierSize> nullifier;
  nullifier.fill(seed);
  nullifier[0] = account_id->account_index;
  return nullifier;
}

OrchardNullifier GenerateMockNullifier(const mojom::AccountIdPtr& account_id,
                                       uint32_t block_id,
                                       uint8_t seed) {
  return OrchardNullifier{block_id, GenerateMockNullifier(account_id, seed)};
}

OrchardNote GenerateMockOrchardNote(const mojom::AccountIdPtr& account_id,
                                    uint32_t block_id,
                                    uint8_t seed) {
  return OrchardNote{block_id, GenerateMockNullifier(account_id, seed),
                     static_cast<uint32_t>(seed * 10)};
}

void SortByBlockId(std::vector<OrchardNote>& vec) {
  std::sort(vec.begin(), vec.end(), [](OrchardNote& a, OrchardNote& b) {
    return (a.block_id < b.block_id);
  });
}

}  // namespace brave_wallet
