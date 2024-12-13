// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_test_utils.h"

#include <memory>
#include <utility>

#include "base/check_is_test.h"
#include "base/memory/ptr_util.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle_impl.h"

namespace brave_wallet::orchard {

class TestingDecodedBundleBuilderImpl : public TestingDecodedBundleBuilder {
 public:
  TestingDecodedBundleBuilderImpl() = default;

  ~TestingDecodedBundleBuilderImpl() override = default;

  void SetPriorTreeState(::brave_wallet::OrchardTreeState tree_state) override {
    prior_tree_state_ = std::move(tree_state);
  }

  void AddCommitment(::brave_wallet::OrchardCommitment commitment) override {
    CxxOrchardCheckpointRetention retention;
    retention.marked = commitment.is_marked;
    retention.checkpoint = commitment.checkpoint_id.has_value();
    retention.checkpoint_id = commitment.checkpoint_id.value_or(0);

    CxxOrchardShardTreeLeaf leaf;
    leaf.hash = commitment.cmu;
    leaf.retention = retention;

    leafs_.commitments.push_back(std::move(leaf));
  }

  std::unique_ptr<OrchardDecodedBlocksBundle> Complete() override {
    ::rust::Vec<uint8_t> frontier;
    base::ranges::copy(prior_tree_state_->frontier,
                       std::back_inserter(frontier));
    auto prior_tree_state =
        CxxOrchardShardTreeState{frontier, prior_tree_state_->block_height,
                                 prior_tree_state_->tree_size};
    return std::make_unique<OrchardDecodedBlocksBundleImpl>(
        base::PassKey<class TestingDecodedBundleBuilderImpl>(),
        create_mock_decode_result(std::move(prior_tree_state),
                                  std::move(leafs_))
            ->unwrap());
  }

 private:
  std::optional<::brave_wallet::OrchardTreeState> prior_tree_state_;
  CxxOrchardShardTreeLeafs leafs_;
};

std::unique_ptr<TestingDecodedBundleBuilder>
CreateTestingDecodedBundleBuilder() {
  CHECK_IS_TEST();
  return std::make_unique<TestingDecodedBundleBuilderImpl>();
}

OrchardCommitmentValue CreateMockCommitmentValue(uint32_t position,
                                                 uint32_t rseed) {
  return create_mock_commitment(position, rseed);
}

}  // namespace brave_wallet::orchard
