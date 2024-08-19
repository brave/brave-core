/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_IMPL_H_

#include <vector>

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_block_decoder.h"

namespace brave_wallet::orchard {

class OrchardBlockDecoderImpl : public OrchardBlockDecoder {
 public:
  ~OrchardBlockDecoderImpl() override;

  std::optional<std::vector<::brave_wallet::OrchardNote>> ScanBlock(
      const ::brave_wallet::zcash::mojom::CompactBlockPtr& block) override;

 private:
  friend class OrchardBlockDecoder;
  explicit OrchardBlockDecoderImpl(const OrchardFullViewKey& fvk);
  OrchardFullViewKey full_view_key_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_BLOCK_DECODER_IMPL_H_
