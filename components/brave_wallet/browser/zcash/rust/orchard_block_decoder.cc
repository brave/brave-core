/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_block_decoder.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle_impl.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

OrchardBlockDecoder::OrchardBlockDecoder() = default;
OrchardBlockDecoder::~OrchardBlockDecoder() = default;

// static
std::unique_ptr<OrchardDecodedBlocksBundle> OrchardBlockDecoder::DecodeBlocks(
    const OrchardFullViewKey& fvk,
    const ::brave_wallet::OrchardTreeState& tree_state,
    const std::vector<::brave_wallet::zcash::mojom::CompactBlockPtr>& blocks) {
  base::AssertLongCPUWorkAllowed();
  ::rust::Vec<orchard::CxxOrchardCompactAction> orchard_actions;
  for (const auto& block : blocks) {
    bool block_has_orchard_action = false;
    for (const auto& tx : block->vtx) {
      for (const auto& orchard_action : tx->orchard_actions) {
        block_has_orchard_action = true;
        orchard::CxxOrchardCompactAction orchard_compact_action;

        if (orchard_action->nullifier.size() != kOrchardNullifierSize ||
            orchard_action->cmx.size() != kOrchardCmxSize ||
            orchard_action->ephemeral_key.size() != kOrchardEphemeralKeySize ||
            orchard_action->ciphertext.size() != kOrchardCipherTextSize) {
          return nullptr;
        }

        orchard_compact_action.block_id = block->height;
        orchard_compact_action.is_block_last_action = false;
        base::span(orchard_compact_action.nullifier)
            .copy_from(orchard_action->nullifier);
        base::span(orchard_compact_action.cmx).copy_from(orchard_action->cmx);
        base::span(orchard_compact_action.ephemeral_key)
            .copy_from(orchard_action->ephemeral_key);
        base::span(orchard_compact_action.enc_cipher_text)
            .copy_from(orchard_action->ciphertext);

        orchard_actions.push_back(std::move(orchard_compact_action));
      }
    }
    if (block_has_orchard_action) {
      orchard_actions.back().is_block_last_action = true;
    }
  }

  CxxOrchardShardTreeState prior_tree_state;
  prior_tree_state.block_height = tree_state.block_height;
  prior_tree_state.tree_size = tree_state.tree_size;

  base::ranges::copy(tree_state.frontier,
                     std::back_inserter(prior_tree_state.frontier));

  ::rust::Box<CxxBatchOrchardDecodeBundleResult> decode_result = batch_decode(
      fvk, std::move(prior_tree_state), std::move(orchard_actions));

  if (decode_result->is_ok()) {
    return base::WrapUnique<OrchardDecodedBlocksBundle>(
        new OrchardDecodedBlocksBundleImpl(decode_result->unwrap()));
  }
  return nullptr;
}

}  // namespace brave_wallet::orchard
