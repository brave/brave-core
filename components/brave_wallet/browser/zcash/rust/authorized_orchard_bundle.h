// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_AUTHORIZED_ORCHARD_BUNDLE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_AUTHORIZED_ORCHARD_BUNDLE_H_

#include <vector>

namespace brave_wallet::orchard {

// Authorized orchard bundle resolved from UnauthorizedOrchardBundle
// after zk-proof based on input data is created and signatures are applied.
// Reference librustzcash flow:
// https://github.com/zcash/librustzcash/blob/5bd911f63bb9b41f97e4b37c32e79b52a7706543/zcash_primitives/src/transaction/builder.rs#L802
class AuthorizedOrchardBundle {
 public:
  virtual ~AuthorizedOrchardBundle() = default;

  // Raw bytes that are used in Zcash nu5 transactions
  // https://zips.z.cash/zip-0225
  virtual std::vector<uint8_t> GetOrchardRawTxPart() = 0;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_AUTHORIZED_ORCHARD_BUNDLE_H_
