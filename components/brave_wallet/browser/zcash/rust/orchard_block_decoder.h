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
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"

namespace brave_wallet::orchard {

class OrchardBlockDecoder {
 public:
  static std::unique_ptr<OrchardDecodedBlocksBundle> DecodeBlocks(
      const OrchardFullViewKey& fvk,
      const ::brave_wallet::OrchardTreeState& tree_state,
      const std::vector<::brave_wallet::zcash::mojom::CompactBlockPtr>& blocks);

 private:
  OrchardBlockDecoder();
  ~OrchardBlockDecoder();
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_H_
