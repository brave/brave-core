/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TEST_UTILS_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

std::array<uint8_t, kOrchardNullifierSize> GenerateMockNullifier(
    const mojom::AccountIdPtr& account_id,
    uint8_t seed);

OrchardNullifier GenerateMockNullifier(const mojom::AccountIdPtr& account_id,
                                       uint32_t block_id,
                                       uint8_t seed);

OrchardNote GenerateMockOrchardNote(const mojom::AccountIdPtr& account_id,
                                    uint32_t block_id,
                                    uint8_t seed);

void SortByBlockId(std::vector<OrchardNote>& vec);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TEST_UTILS_H_
