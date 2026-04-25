// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_TEST_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/internal/orchard_block_scanner.h"

namespace brave_wallet {

OrchardBlockScanner::Result CreateResultForTesting(
    OrchardTreeState tree_state,
    std::vector<OrchardCommitment> commitments,
    uint32_t latest_scanned_block_id,
    const std::string& latest_scanned_block_hash);

OrchardCommitmentValue CreateMockCommitmentValue(uint32_t position,
                                                 uint32_t rseed);

OrchardCommitment CreateCommitment(OrchardCommitmentValue value,
                                   bool marked,
                                   std::optional<uint32_t> checkpoint_id);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_TEST_UTILS_H_
