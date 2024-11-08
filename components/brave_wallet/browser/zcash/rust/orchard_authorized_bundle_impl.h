// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_AUTHORIZED_BUNDLE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_AUTHORIZED_BUNDLE_IMPL_H_

#include <vector>

#include "base/types/pass_key.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_authorized_bundle.h"

namespace brave_wallet::orchard {

class OrchardAuthorizedBundleImpl : public OrchardAuthorizedBundle {
 public:
  OrchardAuthorizedBundleImpl(
      base::PassKey<class OrchardUnauthorizedBundleImpl>,
      ::rust::Box<CxxOrchardAuthorizedBundle>);
  ~OrchardAuthorizedBundleImpl() override;

  std::vector<uint8_t> GetOrchardRawTxPart() override;

 private:
  ::rust::Box<CxxOrchardAuthorizedBundle> cxx_orchard_authorized_bundle_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_AUTHORIZED_BUNDLE_IMPL_H_
