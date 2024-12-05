// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_testing_shard_tree_impl.h"

#include <memory>

#include "brave/components/brave_wallet/browser/zcash/rust/cxx_orchard_shard_tree_delegate.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle_impl.h"

namespace brave_wallet::orchard {

bool OrchardTestingShardTreeImpl::ApplyScanResults(
    std::unique_ptr<OrchardDecodedBlocksBundle> commitments) {
  auto* bundle_impl =
      static_cast<OrchardDecodedBlocksBundleImpl*>(commitments.get());
  return orchard_shard_tree_->insert_commitments(
      bundle_impl->GetDecodeBundle());
}

base::expected<OrchardNoteWitness, std::string>
OrchardTestingShardTreeImpl::CalculateWitness(
    uint32_t note_commitment_tree_position,
    uint32_t checkpoint) {
  auto result = orchard_shard_tree_->calculate_witness(
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

bool OrchardTestingShardTreeImpl::TruncateToCheckpoint(uint32_t checkpoint_id) {
  return orchard_shard_tree_->truncate(checkpoint_id);
}

OrchardTestingShardTreeImpl::OrchardTestingShardTreeImpl(
    base::PassKey<class OrchardShardTree>,
    rust::Box<CxxOrchardTestingShardTreeBundle> orcard_shard_tree)
    : orchard_shard_tree_(std::move(orcard_shard_tree)) {}

OrchardTestingShardTreeImpl::~OrchardTestingShardTreeImpl() {}

// static
std::unique_ptr<OrchardShardTree> OrchardShardTree::CreateForTesting(
    ::brave_wallet::OrchardStorage& storage,
    const mojom::AccountIdPtr& account_id) {
  auto shard_tree_result = create_testing_shard_tree(
      std::make_unique<CxxOrchardShardTreeDelegate>(storage, account_id));
  if (!shard_tree_result->is_ok()) {
    return nullptr;
  }
  return std::make_unique<OrchardTestingShardTreeImpl>(
      base::PassKey<class OrchardShardTree>(), shard_tree_result->unwrap());
}

}  // namespace brave_wallet::orchard
