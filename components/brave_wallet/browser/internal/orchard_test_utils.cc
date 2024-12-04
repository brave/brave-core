// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"

#include <utility>

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_test_utils.h"

namespace brave_wallet {

OrchardBlockScanner::Result CreateResultForTesting(
    OrchardTreeState tree_state,
    std::vector<OrchardCommitment> commitments) {
  auto builder = orchard::CreateTestingDecodedBundleBuilder();
  for (auto& commitment : commitments) {
    builder->AddCommitment(std::move(commitment));
  }
  builder->SetPriorTreeState(std::move(tree_state));
  return OrchardBlockScanner::Result{{}, {}, builder->Complete()};
}

OrchardCommitmentValue CreateMockCommitmentValue(uint32_t position,
                                                 uint32_t rseed) {
  return orchard::CreateMockCommitmentValue(position, rseed);
}

}  // namespace brave_wallet
