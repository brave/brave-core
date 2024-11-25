/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_BLOCK_SCANNER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_BLOCK_SCANNER_H_

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_block_decoder.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

// Scans a bunch of blocks with the provided full view key to find
// spendable notes related to the account.
class OrchardBlockScanner {
 public:
  enum class ErrorCode { kInputError, kDecoderError };

  struct Result {
    Result();
    Result(std::vector<OrchardNote> discovered_notes,
           std::vector<OrchardNoteSpend> spent_notes);
    Result(const Result&);
    Result& operator=(const Result&);
    ~Result();

    // New notes have been discovered
    std::vector<OrchardNote> discovered_notes;
    // Nullifiers for the previously discovered notes
    std::vector<OrchardNoteSpend> spent_notes;
  };

  explicit OrchardBlockScanner(const OrchardFullViewKey& full_view_key);
  virtual ~OrchardBlockScanner();

  // Scans blocks to find incoming notes related to fvk
  // Also checks whether existing notes were spent.
  virtual base::expected<Result, OrchardBlockScanner::ErrorCode> ScanBlocks(
      std::vector<OrchardNote> known_notes,
      std::vector<zcash::mojom::CompactBlockPtr> blocks);

 private:
  std::unique_ptr<orchard::OrchardBlockDecoder> decoder_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_BLOCK_SCANNER_H_
