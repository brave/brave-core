// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_TEST_UTILS_H_

#include <memory>

#include "brave/components/brave_wallet/browser/internal/orchard_block_scanner.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_test_utils.h"

namespace brave_wallet {

class OrchardTestUtils {
 public:
  OrchardTestUtils();
  ~OrchardTestUtils();

  // static
  static OrchardBlockScanner::Result CreateResultForTesting(
      const OrchardTreeState& tree_state,
      const std::vector<OrchardCommitment>& commitments);

  OrchardCommitmentValue CreateMockCommitmentValue(uint32_t position,
                                                   uint32_t rseed);

 private:
  std::unique_ptr<orchard::OrchardTestUtils> orchard_test_utils_impl_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_TEST_UTILS_H_
