// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bunde_impl.h"

#include "base/check_is_test.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

class TestingBuilderImpl : public OrchardDecodedBlocksBundle::TestingBuilder {
 public:
  TestingBuilderImpl() {}

  ~TestingBuilderImpl() override {}

  void SetFrontierChainState(const FrontierChainState& chain_state) override {
    frontier_chain_state_ = chain_state;
  }

  void AddCommitment(
      const ::brave_wallet::OrchardCommitment& commitment) override {
    FfiRetention retention;
    retention.marked = commitment.is_marked;
    retention.checkpoint = commitment.checkpoint_id.has_value();
    retention.checkpoint_id = commitment.checkpoint_id.value_or(0);

    FfiCommitment ffi_commitment;
    ffi_commitment.hash = commitment.cmu;
    ffi_commitment.retention = retention;

    ffi_commitments_.commitments.push_back(std::move(ffi_commitment));
  }

  std::unique_ptr<OrchardDecodedBlocksBundle> Complete() override {
    if (frontier_chain_state_) {
      LOG(ERROR) << "XXXZZZ with frontier "
                 << frontier_chain_state_->frontier_tree_state.size();
      ::rust::Vec<uint8_t> frontier_tree_state;
      base::ranges::copy(frontier_chain_state_->frontier_tree_state,
                         std::back_inserter(frontier_tree_state));
      auto orchard_chain_state = OrchardFrontierChainState{
          frontier_tree_state, frontier_chain_state_->frontier_block_height,
          frontier_chain_state_->frontier_orchard_tree_size};
      return base::WrapUnique<OrchardDecodedBlocksBundle>(
          new OrchardDecodedBlocksBundleImpl(
              create_mock_decode_result_with_frontier(
                  std::move(orchard_chain_state), std::move(ffi_commitments_))
                  ->unwrap()));
    } else {
      return base::WrapUnique<OrchardDecodedBlocksBundle>(
          new OrchardDecodedBlocksBundleImpl(
              create_mock_decode_result(std::move(ffi_commitments_))
                  ->unwrap()));
    }
  }

 private:
  std::optional<FrontierChainState> frontier_chain_state_;
  FfiCommitments ffi_commitments_;
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
