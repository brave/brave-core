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
    absl::variant<base::PassKey<class OrchardBlockDecoder>,
                  base::PassKey<class TestingDecodedBundleBuilderImpl>>,
    rust::Box<CxxOrchardDecodedBlocksBundle> cxx_orchard_decoded_blocks_bundle)
    : cxx_orchard_decoded_blocks_bundle_(
          std::move(cxx_orchard_decoded_blocks_bundle)) {}

OrchardDecodedBlocksBundleImpl::~OrchardDecodedBlocksBundleImpl() = default;

std::optional<std::vector<::brave_wallet::OrchardNote>>
OrchardDecodedBlocksBundleImpl::GetDiscoveredNotes() {
  std::vector<OrchardNote> result;
  result.reserve(cxx_orchard_decoded_blocks_bundle_->size());
  for (size_t i = 0; i < cxx_orchard_decoded_blocks_bundle_->size(); i++) {
    result.push_back(OrchardNote{
        cxx_orchard_decoded_blocks_bundle_->note_addr(i),
        cxx_orchard_decoded_blocks_bundle_->note_block_height(i),
        cxx_orchard_decoded_blocks_bundle_->note_nullifier(i),
        cxx_orchard_decoded_blocks_bundle_->note_value(i),
        cxx_orchard_decoded_blocks_bundle_->note_commitment_tree_position(i),
        cxx_orchard_decoded_blocks_bundle_->note_rho(i),
        cxx_orchard_decoded_blocks_bundle_->note_rseed(i),
    });
  }

  return result;
}

CxxOrchardDecodedBlocksBundle&
OrchardDecodedBlocksBundleImpl::GetDecodeBundle() {
  return *cxx_orchard_decoded_blocks_bundle_;
}

}  // namespace brave_wallet::orchard
