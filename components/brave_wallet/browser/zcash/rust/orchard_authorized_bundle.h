// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_AUTHORIZED_BUNDLE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_AUTHORIZED_BUNDLE_H_

#include <vector>

namespace brave_wallet::orchard {

// Authorized orchard bundle resolved from OrchardUnauthorizedBundle
// after zk-proof based on input data is created and signatures are applied.
// References to the Bundle in the Orchard crate with Authorized state:
// https://github.com/zcash/orchard/blob/23a167e3972632586dc628ddbdd69d156dfd607b/src/bundle.rs#L152
// Reference for the authorization librustzcash flow:
// https://github.com/zcash/librustzcash/blob/5bd911f63bb9b41f97e4b37c32e79b52a7706543/zcash_primitives/src/transaction/builder.rs#L802
class OrchardAuthorizedBundle {
 public:
  virtual ~OrchardAuthorizedBundle() = default;

  // Raw bytes that are used in Zcash nu5 transactions
  // https://zips.z.cash/zip-0225
  virtual std::vector<uint8_t> GetOrchardRawTxPart() = 0;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_AUTHORIZED_BUNDLE_H_
