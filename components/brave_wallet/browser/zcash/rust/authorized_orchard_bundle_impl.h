// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_AUTHORIZED_ORCHARD_BUNDLE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_AUTHORIZED_ORCHARD_BUNDLE_IMPL_H_

#include <vector>

#include "brave/components/brave_wallet/browser/zcash/rust/authorized_orchard_bundle.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"

namespace brave_wallet::orchard {

class AuthorizedOrchardBundleImpl : public AuthorizedOrchardBundle {
 public:
  ~AuthorizedOrchardBundleImpl() override;

  std::vector<uint8_t> GetOrchardRawTxPart() override;

 private:
  explicit AuthorizedOrchardBundleImpl(
      ::rust::Box<orchard::OrchardAuthorizedBundle> orchard_authorized_bundle);
  friend class UnauthorizedOrchardBundleImpl;

  ::rust::Box<orchard::OrchardAuthorizedBundle> orchard_authorized_bundle_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_AUTHORIZED_ORCHARD_BUNDLE_IMPL_H_
