/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_block_scanner.h"

namespace brave_wallet {

OrchardBlockScanner::Result::Result() = default;

OrchardBlockScanner::Result::Result(std::vector<OrchardNote> discovered_notes,
                                    std::vector<OrchardNullifier> spent_notes,
                                    std::vector<OrchardCommitment> commitments)
    : discovered_notes(std::move(discovered_notes)),
      spent_notes(std::move(spent_notes)),
      commitments(std::move(commitments)) {}

OrchardBlockScanner::Result::Result(const Result&) = default;

OrchardBlockScanner::Result& OrchardBlockScanner::Result::operator=(
    const Result&) = default;

OrchardBlockScanner::Result::~Result() = default;

OrchardBlockScanner::OrchardBlockScanner(
    const std::array<uint8_t, kOrchardFullViewKeySize>& full_view_key)
    : decoder_(orchard::OrchardBlockDecoder::FromFullViewKey(full_view_key)) {}

OrchardBlockScanner::~OrchardBlockScanner() = default;

base::expected<OrchardBlockScanner::Result, OrchardBlockScanner::ErrorCode>
OrchardBlockScanner::ScanBlocks(
    std::vector<OrchardNote> known_notes,
    std::vector<zcash::mojom::CompactBlockPtr> blocks) {
  std::vector<OrchardNullifier> found_nullifiers;
  std::vector<OrchardNote> found_notes;
  std::vector<OrchardCommitment> commitments;

  uint32_t cmu_index = 0;
  for (const auto& block : blocks) {
    // Scan block using the decoder initialized with the provided fvk
    // to find new spendable notes.
    auto scan_result = decoder_->ScanBlock(block);
    if (!scan_result) {
      return base::unexpected(ErrorCode::kDecoderError);
    }
    found_notes.insert(found_notes.end(), scan_result->begin(),
                       scan_result->end());
    // Place found notes to the known notes list so we can also check for
    // nullifiers
    known_notes.insert(known_notes.end(), scan_result->begin(),
                       scan_result->end());
    for (const auto& tx : block->vtx) {
      // We only scan orchard actions here
      for (const auto& orchard_action : tx->orchard_actions) {
        if (orchard_action->nullifier.size() != kOrchardNullifierSize) {
          return base::unexpected(ErrorCode::kInputError);
        }

        if (orchard_action->cmx.size() != kOrchardCmxSize) {
          return base::unexpected(ErrorCode::kInputError);
        }

        OrchardCommitment commitment;
        base::ranges::copy(orchard_action->cmx.begin(),
                           orchard_action->cmx.end(), commitment.cmu.begin());
        commitment.block_id = block->height;
        commitment.index = cmu_index++;

        std::array<uint8_t, kOrchardNullifierSize> action_nullifier;
        std::copy(orchard_action->nullifier.begin(),
                  orchard_action->nullifier.end(), action_nullifier.begin());

        // Nullifier is a public information about some note being spent.
        // Here we are trying to find a known spendable notes which nullifier
        // matches nullifier from the processed transaction.
        if (std::find_if(known_notes.begin(), known_notes.end(),
                         [&action_nullifier](const auto& v) {
                           return v.nullifier == action_nullifier;
                         }) != known_notes.end()) {
          OrchardNullifier nullifier;
          nullifier.block_id = block->height;
          nullifier.nullifier = action_nullifier;
          found_nullifiers.push_back(std::move(nullifier));
        }
      }
    }
  }
  return Result({std::move(found_notes), std::move(found_nullifiers),
                 std::move(commitments)});
}

}  // namespace brave_wallet
