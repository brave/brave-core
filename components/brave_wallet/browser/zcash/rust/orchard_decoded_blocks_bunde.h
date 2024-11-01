// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_DECODED_BLOCKS_BUNDE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_DECODED_BLOCKS_BUNDE_H_

#include <memory>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

class OrchardDecodedBlocksBundle {
 public:
  // Builder is used in tests to create OrchardDecodedBlocksBundle with mocked
  // commitments
  class TestingBuilder {
   public:
    TestingBuilder() = default;
    virtual ~TestingBuilder() = default;
    virtual void AddCommitment(
        const ::brave_wallet::OrchardCommitment& commitment) = 0;
    virtual void SetPriorTreeState(
        const ::brave_wallet::OrchardTreeState& tree_state) = 0;
    virtual std::unique_ptr<OrchardDecodedBlocksBundle> Complete() = 0;
  };

  static std::unique_ptr<TestingBuilder> CreateTestingBuilder();

  virtual ~OrchardDecodedBlocksBundle() {}
  virtual std::optional<std::vector<::brave_wallet::OrchardNote>>
  GetDiscoveredNotes() = 0;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_DECODED_BLOCKS_BUNDE_H_
