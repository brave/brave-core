// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_AUTHORIZED_ORCHARD_BUNDLE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_AUTHORIZED_ORCHARD_BUNDLE_H_

#include <vector>

namespace brave_wallet::orchard {

class AuthorizedOrchardBundle {
 public:
  virtual ~AuthorizedOrchardBundle() = default;

  virtual std::vector<uint8_t> GetOrchardRawTxPart() = 0;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_AUTHORIZED_ORCHARD_BUNDLE_H_
