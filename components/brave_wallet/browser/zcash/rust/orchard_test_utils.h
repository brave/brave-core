// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_TEST_UTILS_H_

#include <memory>

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_shard_tree_types.h"

namespace brave_wallet::orchard {

class OrchardDecodedBlocksBundle;

// Builder is used in tests to create OrchardDecodedBlocksBundle with mocked
// commitments
class TestingDecodedBundleBuilder {
 public:
  TestingDecodedBundleBuilder() = default;
  virtual ~TestingDecodedBundleBuilder() = default;
  virtual void AddCommitment(::brave_wallet::OrchardCommitment commitment) = 0;
  virtual void SetPriorTreeState(
      ::brave_wallet::OrchardTreeState tree_state) = 0;
  virtual std::unique_ptr<OrchardDecodedBlocksBundle> Complete() = 0;
};

std::unique_ptr<TestingDecodedBundleBuilder>
CreateTestingDecodedBundleBuilder();

OrchardCommitmentValue CreateMockCommitmentValue(uint32_t position,
                                                 uint32_t rseed);

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_TEST_UTILS_H_
