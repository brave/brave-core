// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_DECODED_BLOCKS_BUNDLE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_DECODED_BLOCKS_BUNDLE_IMPL_H_

#include <vector>

#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::orchard {

class OrchardDecodedBlocksBundleImpl : public OrchardDecodedBlocksBundle {
 public:
  ~OrchardDecodedBlocksBundleImpl() override;

  std::optional<std::vector<::brave_wallet::OrchardNote>> GetDiscoveredNotes()
      override;
  CxxBatchOrchardDecodeBundle& GetDecodeBundle();

 private:
  friend class OrchardBlockDecoder;
  friend class TestingDecodedBundleBuilderImpl;
  explicit OrchardDecodedBlocksBundleImpl(
      rust::Box<CxxBatchOrchardDecodeBundle>);
  rust::Box<CxxBatchOrchardDecodeBundle> batch_decode_result_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_DECODED_BLOCKS_BUNDLE_IMPL_H_
