/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_H_

#include <memory>
#include <optional>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"

namespace brave_wallet::orchard {

class OrchardBlockDecoder {
 public:
  // Result of decoding one batch of blocks for both pools.
  struct Result {
    Result();
    ~Result();
    Result(Result&&);
    Result& operator=(Result&&);

    // Non-null on success. Null means the Orchard decode failed.
    std::unique_ptr<OrchardDecodedBlocksBundle> orchard;
    // Null when Ironwood was not requested (or decode failed).
    std::unique_ptr<OrchardDecodedBlocksBundle> ironwood;
  };

  static Result DecodeBlocks(
      const OrchardFullViewKey& fvk,
      const ::brave_wallet::OrchardTreeState& orchard_tree_state,
      const std::vector<::brave_wallet::zcash::mojom::CompactBlockPtr>& blocks,
      const ::brave_wallet::OrchardTreeState* ironwood_tree_state = nullptr);

 private:
  OrchardBlockDecoder();
  ~OrchardBlockDecoder();

  // Decodes a single pool. Selects tx->orchard_actions vs tx->ironwood_actions
  // based on `pool`, then calls the (shared) Rust batch_decode. Must be a
  // member so it can mint base::PassKey<OrchardBlockDecoder>.
  static std::unique_ptr<OrchardDecodedBlocksBundle> DecodePool(
      const OrchardFullViewKey& fvk,
      const ::brave_wallet::OrchardTreeState& tree_state,
      const std::vector<::brave_wallet::zcash::mojom::CompactBlockPtr>& blocks,
      ::brave_wallet::OrchardPool pool);
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_H_
