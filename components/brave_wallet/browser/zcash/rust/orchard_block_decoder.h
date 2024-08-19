/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_H_

#include <memory>
#include <optional>
#include <vector>

#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"

namespace brave_wallet::orchard {

class OrchardBlockDecoder {
 public:
  virtual ~OrchardBlockDecoder() = default;

  virtual std::optional<std::vector<::brave_wallet::OrchardNote>> ScanBlock(
      const ::brave_wallet::zcash::mojom::CompactBlockPtr& block) = 0;

  static std::unique_ptr<OrchardBlockDecoder> FromFullViewKey(
      const OrchardFullViewKey& fvk);
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_H_
