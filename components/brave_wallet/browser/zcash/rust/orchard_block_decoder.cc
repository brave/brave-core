/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_block_decoder.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle_impl.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

OrchardBlockDecoder::Result::Result() = default;
OrchardBlockDecoder::Result::~Result() = default;
OrchardBlockDecoder::Result::Result(Result&&) = default;
OrchardBlockDecoder::Result& OrchardBlockDecoder::Result::operator=(Result&&) =
    default;

OrchardBlockDecoder::OrchardBlockDecoder() = default;
OrchardBlockDecoder::~OrchardBlockDecoder() = default;

// static
std::unique_ptr<OrchardDecodedBlocksBundle> OrchardBlockDecoder::DecodePool(
    const OrchardFullViewKey& fvk,
    const ::brave_wallet::OrchardTreeState& tree_state,
    const std::vector<::brave_wallet::zcash::mojom::CompactBlockPtr>& blocks,
    ::brave_wallet::OrchardPool pool) {
  const bool is_ironwood = pool == ::brave_wallet::OrchardPool::kIronwood;
  ::rust::Vec<orchard::CxxOrchardCompactAction> compact_actions;
  for (const auto& block : blocks) {
    bool block_has_action = false;
    for (const auto& tx : block->vtx) {
      const auto& actions =
          is_ironwood ? tx->ironwood_actions : tx->orchard_actions;
      for (const auto& action : actions) {
        if (action->nullifier.size() != kOrchardNullifierSize ||
            action->cmx.size() != kOrchardCmxSize ||
            action->ephemeral_key.size() != kOrchardEphemeralKeySize ||
            action->ciphertext.size() != kOrchardCipherTextSize) {
          return nullptr;
        }
        block_has_action = true;
        orchard::CxxOrchardCompactAction c;
        c.block_id = block->height;
        c.is_block_last_action = false;
        base::span(c.nullifier).copy_from(action->nullifier);
        base::span(c.cmx).copy_from(action->cmx);
        base::span(c.ephemeral_key).copy_from(action->ephemeral_key);
        base::span(c.enc_cipher_text).copy_from(action->ciphertext);
        compact_actions.push_back(std::move(c));
      }
    }
    if (block_has_action) {
      compact_actions.back().is_block_last_action = true;
    }
  }

  CxxOrchardShardTreeState prior_tree_state;
  prior_tree_state.block_height = tree_state.block_height;
  prior_tree_state.tree_size = tree_state.tree_size;
  std::ranges::copy(tree_state.frontier,
                    std::back_inserter(prior_tree_state.frontier));

  ::rust::Box<CxxOrchardDecodedBlocksBundleResult> decode_result =
      is_ironwood
          ? batch_decode_ironwood(fvk, std::move(prior_tree_state),
                                  std::move(compact_actions))
          : batch_decode(fvk, std::move(prior_tree_state),
                         std::move(compact_actions));
  if (decode_result->is_ok()) {
    auto bundle = decode_result->unwrap();
    return std::make_unique<OrchardDecodedBlocksBundleImpl>(
        base::PassKey<class OrchardBlockDecoder>(), std::move(bundle));
  }
  return nullptr;
}

// static
OrchardBlockDecoder::Result OrchardBlockDecoder::DecodeBlocks(
    const OrchardFullViewKey& fvk,
    const ::brave_wallet::OrchardTreeState& orchard_tree_state,
    const std::vector<::brave_wallet::zcash::mojom::CompactBlockPtr>& blocks,
    const ::brave_wallet::OrchardTreeState* ironwood_tree_state) {
  base::AssertLongCPUWorkAllowed();
  Result result;
  result.orchard =
      DecodePool(fvk, orchard_tree_state, blocks, OrchardPool::kOrchard);
  if (ironwood_tree_state) {
    result.ironwood =
        DecodePool(fvk, *ironwood_tree_state, blocks, OrchardPool::kIronwood);
  }
  return result;
}

}  // namespace brave_wallet::orchard
