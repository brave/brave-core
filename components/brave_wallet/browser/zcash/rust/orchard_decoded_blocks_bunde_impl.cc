// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bunde_impl.h"

#include <memory>
#include <utility>

#include "base/check_is_test.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

class TestingBuilderImpl : public OrchardDecodedBlocksBundle::TestingBuilder {
 public:
  TestingBuilderImpl() {}

  ~TestingBuilderImpl() override {}

  void SetPriorTreeState(
      const ::brave_wallet::OrchardTreeState& tree_state) override {
    prior_tree_state_ = tree_state;
  }

  void AddCommitment(
      const ::brave_wallet::OrchardCommitment& commitment) override {
    ShardTreeCheckpointRetention retention;
    retention.marked = commitment.is_marked;
    retention.checkpoint = commitment.checkpoint_id.has_value();
    retention.checkpoint_id = commitment.checkpoint_id.value_or(0);

    ShardTreeLeaf leaf;
    leaf.hash = commitment.cmu;
    leaf.retention = retention;

    leafs_.commitments.push_back(std::move(leaf));
  }

  std::unique_ptr<OrchardDecodedBlocksBundle> Complete() override {
    ::rust::Vec<uint8_t> frontier;
    base::ranges::copy(prior_tree_state_->frontier,
                       std::back_inserter(frontier));
    auto prior_tree_state =
        ShardTreeState{frontier, prior_tree_state_->block_height,
                       prior_tree_state_->tree_size};
    return base::WrapUnique<OrchardDecodedBlocksBundle>(
        new OrchardDecodedBlocksBundleImpl(
            create_mock_decode_result(std::move(prior_tree_state),
                                      std::move(leafs_))
                ->unwrap()));
  }

 private:
  std::optional<::brave_wallet::OrchardTreeState> prior_tree_state_;
  ShardTreeLeafs leafs_;
};

OrchardDecodedBlocksBundleImpl::OrchardDecodedBlocksBundleImpl(
    rust::Box<BatchOrchardDecodeBundle> v)
    : batch_decode_result_(std::move(v)) {}

OrchardDecodedBlocksBundleImpl::~OrchardDecodedBlocksBundleImpl() {}

std::optional<std::vector<::brave_wallet::OrchardNote>>
OrchardDecodedBlocksBundleImpl::GetDiscoveredNotes() {
  std::vector<OrchardNote> result;

  for (size_t i = 0; i < batch_decode_result_->size(); i++) {
    result.emplace_back(OrchardNote({
        batch_decode_result_->note_addr(i),
        batch_decode_result_->note_block_height(i),
        batch_decode_result_->note_nullifier(i),
        batch_decode_result_->note_value(i),
        batch_decode_result_->note_commitment_tree_position(i),
        batch_decode_result_->note_rho(i),
        batch_decode_result_->note_rseed(i),
    }));
  }

  return result;
}

BatchOrchardDecodeBundle& OrchardDecodedBlocksBundleImpl::GetDecodeBundle() {
  return *batch_decode_result_;
}

// static
std::unique_ptr<OrchardDecodedBlocksBundle::TestingBuilder>
OrchardDecodedBlocksBundle::CreateTestingBuilder() {
  CHECK_IS_TEST();
  return base::WrapUnique<TestingBuilder>(new TestingBuilderImpl());
}

}  // namespace brave_wallet::orchard
