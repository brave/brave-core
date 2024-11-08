/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_block_scanner.h"

#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_block_decoder.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle.h"

namespace brave_wallet {

OrchardBlockScanner::Result::Result() = default;

OrchardBlockScanner::Result::Result(
    std::vector<OrchardNote> discovered_notes,
    std::vector<OrchardNoteSpend> spent_notes,
    std::unique_ptr<orchard::OrchardDecodedBlocksBundle> scanned_blocks)
    : discovered_notes(std::move(discovered_notes)),
      found_spends(std::move(spent_notes)),
      scanned_blocks(std::move(scanned_blocks)) {}

OrchardBlockScanner::Result::Result(OrchardBlockScanner::Result&&) = default;
OrchardBlockScanner::Result& OrchardBlockScanner::Result::operator=(
    OrchardBlockScanner::Result&&) = default;

OrchardBlockScanner::Result::~Result() = default;

OrchardBlockScanner::OrchardBlockScanner(const OrchardFullViewKey& fvk)
    : fvk_(fvk) {}

OrchardBlockScanner::~OrchardBlockScanner() = default;

base::expected<OrchardBlockScanner::Result, OrchardBlockScanner::ErrorCode>
OrchardBlockScanner::ScanBlocks(
    const OrchardTreeState& tree_state,
    const std::vector<zcash::mojom::CompactBlockPtr>& blocks) {
  base::AssertLongCPUWorkAllowed();

  std::unique_ptr<orchard::OrchardDecodedBlocksBundle> result =
      orchard::OrchardBlockDecoder::DecodeBlocks(fvk_, tree_state, blocks);
  if (!result) {
    return base::unexpected(ErrorCode::kInputError);
  }

  std::optional<std::vector<OrchardNote>> found_notes =
      result->GetDiscoveredNotes();

  if (!found_notes) {
    return base::unexpected(ErrorCode::kDiscoveredNotesError);
  }

  std::vector<OrchardNoteSpend> found_spends;

  for (const auto& block : blocks) {
    for (const auto& tx : block->vtx) {
      // We only scan orchard actions here
      for (const auto& orchard_action : tx->orchard_actions) {
        if (orchard_action->nullifier.size() != kOrchardNullifierSize) {
          return base::unexpected(ErrorCode::kInputError);
        }

        // Nullifier is a public information about some note being spent.
        // Here we are collecting nullifiers from the blocks to check them
        // later.
        OrchardNoteSpend spend;
        base::span(spend.nullifier).copy_from(orchard_action->nullifier);
        spend.block_id = block->height;
        found_spends.push_back(std::move(spend));
      }
    }
  }

  return Result({std::move(found_notes.value()), std::move(found_spends),
                 std::move(result)});
}

}  // namespace brave_wallet
