/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_wallet/browser/zcash/rust/cxx_orchard_shard_tree_delegate.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle_impl.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

class OrchardShardTreeImpl : public OrchardShardTree {
 public:
  OrchardShardTreeImpl(base::PassKey<class OrchardShardTree>,
                       rust::Box<CxxOrchardShardTree> orcard_shard_tree);
  ~OrchardShardTreeImpl() override;

  bool TruncateToCheckpoint(uint32_t checkpoint_id) override;

  bool ApplyScanResults(
      std::unique_ptr<OrchardDecodedBlocksBundle> commitments) override;

  base::expected<OrchardNoteWitness, std::string> CalculateWitness(
      uint32_t note_commitment_tree_position,
      uint32_t checkpoint) override;

 private:
  ::rust::Box<CxxOrchardShardTree> cxx_orchard_shard_tree_;
};

bool OrchardShardTreeImpl::ApplyScanResults(
    std::unique_ptr<OrchardDecodedBlocksBundle> commitments) {
  auto* bundle_impl =
      static_cast<OrchardDecodedBlocksBundleImpl*>(commitments.get());
  return cxx_orchard_shard_tree_->insert_commitments(
      bundle_impl->GetDecodeBundle());
}

base::expected<OrchardNoteWitness, std::string>
OrchardShardTreeImpl::CalculateWitness(uint32_t note_commitment_tree_position,
                                       uint32_t checkpoint) {
  auto result = cxx_orchard_shard_tree_->calculate_witness(
      note_commitment_tree_position, checkpoint);
  if (!result->is_ok()) {
    return base::unexpected(result->error_message().c_str());
  }

  auto value = result->unwrap();

  OrchardNoteWitness witness;
  witness.position = note_commitment_tree_position;
  for (size_t i = 0; i < value->size(); i++) {
    witness.merkle_path.push_back(value->item(i));
  }

  return witness;
}

bool OrchardShardTreeImpl::TruncateToCheckpoint(uint32_t checkpoint_id) {
  return cxx_orchard_shard_tree_->truncate(checkpoint_id);
}

OrchardShardTreeImpl::OrchardShardTreeImpl(
    base::PassKey<class OrchardShardTree>,
    ::rust::Box<CxxOrchardShardTree> orcard_shard_tree)
    : cxx_orchard_shard_tree_(std::move(orcard_shard_tree)) {}

OrchardShardTreeImpl::~OrchardShardTreeImpl() {}

// static
std::unique_ptr<OrchardShardTree> OrchardShardTree::Create(
    ::brave_wallet::OrchardStorage& storage,
    const mojom::AccountIdPtr& account_id) {
  auto orchard_shard_tree_result = create_orchard_shard_tree(
      std::make_unique<CxxOrchardShardTreeDelegate>(storage, account_id));
  if (!orchard_shard_tree_result->is_ok()) {
    return nullptr;
  }
  return std::make_unique<OrchardShardTreeImpl>(
      base::PassKey<class OrchardShardTree>(),
      orchard_shard_tree_result->unwrap());
}

}  // namespace brave_wallet::orchard
