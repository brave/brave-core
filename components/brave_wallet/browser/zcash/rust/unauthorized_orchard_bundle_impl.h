/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_UNAUTHORIZED_ORCHARD_BUNDLE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_UNAUTHORIZED_ORCHARD_BUNDLE_IMPL_H_

#include <memory>

#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/unauthorized_orchard_bundle.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::orchard {

class UnauthorizedOrchardBundleImpl : public UnauthorizedOrchardBundle {
 public:
  ~UnauthorizedOrchardBundleImpl() override;

  std::array<uint8_t, kZCashDigestSize> GetDigest() override;
  std::unique_ptr<AuthorizedOrchardBundle> Complete(
      const std::array<uint8_t, kZCashDigestSize>& sighash) override;

 private:
  friend class UnauthorizedOrchardBundle;
  explicit UnauthorizedOrchardBundleImpl(
      ::rust::Box<orchard::OrchardUnauthorizedBundle>
          orchard_unauthorized_bundle);

  ::rust::Box<orchard::OrchardUnauthorizedBundle> orchard_unauthorized_bundle_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_UNAUTHORIZED_ORCHARD_BUNDLE_IMPL_H_
