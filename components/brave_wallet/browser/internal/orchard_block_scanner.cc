/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_block_scanner.h"

#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_block_decoder.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {

std::optional<std::vector<OrchardNoteSpend>> CollectSpends(
    const std::vector<zcash::mojom::CompactBlockPtr>& blocks,
    OrchardPool pool) {
  std::vector<OrchardNoteSpend> found_spends;
  for (const auto& block : blocks) {
    for (const auto& tx : block->vtx) {
      const auto& actions = pool == OrchardPool::kIronwood
                                ? tx->ironwood_actions
                                : tx->orchard_actions;
      for (const auto& action : actions) {
        if (action->nullifier.size() != kOrchardNullifierSize) {
          return std::nullopt;
        }
        OrchardNoteSpend spend;
        base::span(spend.nullifier).copy_from(action->nullifier);
        spend.block_id = block->height;
        found_spends.push_back(std::move(spend));
      }
    }
  }
  return found_spends;
}

base::expected<OrchardBlockScanner::PoolResult, OrchardBlockScanner::ErrorCode>
BuildPoolResult(
    std::unique_ptr<orchard::OrchardDecodedBlocksBundle> bundle,
    const std::vector<zcash::mojom::CompactBlockPtr>& blocks,
    OrchardPool pool) {
  if (!bundle) {
    return base::unexpected(OrchardBlockScanner::ErrorCode::kInputError);
  }
  auto notes = bundle->GetDiscoveredNotes();
  if (!notes) {
    return base::unexpected(
        OrchardBlockScanner::ErrorCode::kDiscoveredNotesError);
  }
  auto spends = CollectSpends(blocks, pool);
  if (!spends) {
    return base::unexpected(OrchardBlockScanner::ErrorCode::kInputError);
  }
  return OrchardBlockScanner::PoolResult(
      std::move(notes.value()), std::move(spends.value()), std::move(bundle),
      blocks.back()->height, ToHex(blocks.back()->hash));
}

}  // namespace

OrchardBlockScanner::PoolResult::PoolResult() = default;

OrchardBlockScanner::PoolResult::PoolResult(
    std::vector<OrchardNote> discovered_notes,
    std::vector<OrchardNoteSpend> spent_notes,
    std::unique_ptr<orchard::OrchardDecodedBlocksBundle> scanned_blocks,
    uint32_t latest_scanned_block_id,
    const std::string& latest_scanned_block_hash)
    : discovered_notes(std::move(discovered_notes)),
      found_spends(std::move(spent_notes)),
      scanned_blocks(std::move(scanned_blocks)),
      latest_scanned_block_id(latest_scanned_block_id),
      latest_scanned_block_hash(latest_scanned_block_hash) {}

OrchardBlockScanner::PoolResult::PoolResult(
    OrchardBlockScanner::PoolResult&&) = default;
OrchardBlockScanner::PoolResult& OrchardBlockScanner::PoolResult::operator=(
    OrchardBlockScanner::PoolResult&&) = default;

OrchardBlockScanner::PoolResult::~PoolResult() = default;

OrchardBlockScanner::Result::Result() = default;
OrchardBlockScanner::Result::Result(OrchardBlockScanner::Result&&) = default;
OrchardBlockScanner::Result& OrchardBlockScanner::Result::operator=(
    OrchardBlockScanner::Result&&) = default;
OrchardBlockScanner::Result::~Result() = default;

OrchardBlockScanner::OrchardBlockScanner(const OrchardFullViewKey& fvk)
    : fvk_(fvk) {}

OrchardBlockScanner::~OrchardBlockScanner() = default;

base::expected<OrchardBlockScanner::Result, OrchardBlockScanner::ErrorCode>
OrchardBlockScanner::ScanBlocks(
    const OrchardTreeState& orchard_tree_state,
    const std::vector<zcash::mojom::CompactBlockPtr>& blocks,
    const OrchardTreeState* ironwood_tree_state) {
  base::AssertLongCPUWorkAllowed();
  if (blocks.empty()) {
    return base::unexpected(ErrorCode::kInputError);
  }

  const bool decode_ironwood =
      IsZCashIronwoodTransactionEnabled() && ironwood_tree_state != nullptr;

  orchard::OrchardBlockDecoder::Result decoded =
      orchard::OrchardBlockDecoder::DecodeBlocks(
          fvk_, orchard_tree_state, blocks,
          decode_ironwood ? ironwood_tree_state : nullptr);

  auto orchard_result =
      BuildPoolResult(std::move(decoded.orchard), blocks, OrchardPool::kOrchard);
  if (!orchard_result.has_value()) {
    return base::unexpected(orchard_result.error());
  }

  Result result;
  result.orchard = std::move(orchard_result.value());

  if (decode_ironwood) {
    auto ironwood_result = BuildPoolResult(std::move(decoded.ironwood), blocks,
                                           OrchardPool::kIronwood);
    if (!ironwood_result.has_value()) {
      return base::unexpected(ironwood_result.error());
    }
    result.ironwood = std::move(ironwood_result.value());
  }
  return result;
}

}  // namespace brave_wallet
