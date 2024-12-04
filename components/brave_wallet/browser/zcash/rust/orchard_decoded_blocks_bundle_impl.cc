// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle_impl.h"

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

OrchardDecodedBlocksBundleImpl::OrchardDecodedBlocksBundleImpl(
    rust::Box<CxxBatchOrchardDecodeBundle> v)
    : batch_decode_result_(std::move(v)) {}

OrchardDecodedBlocksBundleImpl::~OrchardDecodedBlocksBundleImpl() = default;

std::optional<std::vector<::brave_wallet::OrchardNote>>
OrchardDecodedBlocksBundleImpl::GetDiscoveredNotes() {
  std::vector<OrchardNote> result;
  result.reserve(batch_decode_result_->size());
  for (size_t i = 0; i < batch_decode_result_->size(); i++) {
    result.push_back(OrchardNote{
        batch_decode_result_->note_addr(i),
        batch_decode_result_->note_block_height(i),
        batch_decode_result_->note_nullifier(i),
        batch_decode_result_->note_value(i),
        batch_decode_result_->note_commitment_tree_position(i),
        batch_decode_result_->note_rho(i),
        batch_decode_result_->note_rseed(i),
    });
  }

  return result;
}

CxxBatchOrchardDecodeBundle& OrchardDecodedBlocksBundleImpl::GetDecodeBundle() {
  return *batch_decode_result_;
}

}  // namespace brave_wallet::orchard
