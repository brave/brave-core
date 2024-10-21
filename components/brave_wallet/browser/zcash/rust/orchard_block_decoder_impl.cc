/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_block_decoder_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bunde_impl.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

OrchardBlockDecoderImpl::OrchardBlockDecoderImpl(const OrchardFullViewKey& fvk)
    : full_view_key_(fvk) {}

OrchardBlockDecoderImpl::~OrchardBlockDecoderImpl() = default;

std::unique_ptr<OrchardDecodedBlocksBundle> OrchardBlockDecoderImpl::ScanBlocks(
    const FrontierChainState& frontier_chain_state,
    const std::vector<::brave_wallet::zcash::mojom::CompactBlockPtr>& blocks) {
  LOG(ERROR) << "XXXZZZ ScanBlocks";
  ::rust::Vec<orchard::OrchardCompactAction> orchard_actions;
  for (const auto& block : blocks) {
    LOG(ERROR) << "XXXZZZ ScanBlock " << block->height;
    bool block_has_orchard_action = false;
    for (const auto& tx : block->vtx) {
      for (const auto& orchard_action : tx->orchard_actions) {
        LOG(ERROR) << "XXXZZZ ScanBlocks action";

        block_has_orchard_action = true;
        orchard::OrchardCompactAction orchard_compact_action;

        if (orchard_action->nullifier.size() != kOrchardNullifierSize ||
            orchard_action->cmx.size() != kOrchardCmxSize ||
            orchard_action->ephemeral_key.size() != kOrchardEphemeralKeySize ||
            orchard_action->ciphertext.size() != kOrchardCipherTextSize) {
          return nullptr;
        }

        orchard_compact_action.block_id = block->height;
        orchard_compact_action.is_block_last_action = false;
        base::ranges::copy(orchard_action->nullifier,
                           orchard_compact_action.nullifier.begin());
        base::ranges::copy(orchard_action->cmx,
                           orchard_compact_action.cmx.begin());
        base::ranges::copy(orchard_action->ephemeral_key,
                           orchard_compact_action.ephemeral_key.begin());
        base::ranges::copy(orchard_action->ciphertext,
                           orchard_compact_action.enc_cipher_text.begin());

        orchard_actions.push_back(std::move(orchard_compact_action));
      }
    }
    if (block_has_orchard_action) {
      LOG(ERROR) << "XXXZZZ set block has last action";
      orchard_actions.back().is_block_last_action = true;
    }
  }

  ::brave_wallet::orchard::OrchardFrontierChainState chain_state;
  chain_state.frontier_block_height =
      frontier_chain_state.frontier_block_height;
  chain_state.frontier_orchard_commitment_tree_size =
      frontier_chain_state.frontier_orchard_tree_size;

  base::ranges::copy(frontier_chain_state.frontier_tree_state,
                     std::back_inserter(chain_state.frontier_tree_state));
  LOG(ERROR) << "XXXZZZ frontier_block_height "
             << frontier_chain_state.frontier_block_height;
  LOG(ERROR) << "XXXZZZ frontier_orchard_tree_size "
             << frontier_chain_state.frontier_orchard_tree_size;

  LOG(ERROR) << "XXXZZZ batch_decode actions count: " << orchard_actions.size();
  ::rust::Box<::brave_wallet::orchard::BatchOrchardDecodeBundleResult>
      decode_result = ::brave_wallet::orchard::batch_decode(
          full_view_key_, std::move(chain_state), std::move(orchard_actions));

  if (decode_result->is_ok()) {
    return base::WrapUnique<OrchardDecodedBlocksBundle>(
        new OrchardDecodedBlocksBundleImpl(decode_result->unwrap()));
  } else {
    return nullptr;
  }
}

// static
std::unique_ptr<OrchardBlockDecoder> OrchardBlockDecoder::FromFullViewKey(
    const OrchardFullViewKey& fvk) {
  return base::WrapUnique<OrchardBlockDecoder>(
      new OrchardBlockDecoderImpl(fvk));
}

}  // namespace brave_wallet::orchard
