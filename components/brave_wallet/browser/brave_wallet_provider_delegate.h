/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_

namespace brave_wallet {

class BraveWalletProviderDelegate {
 public:
  BraveWalletProviderDelegate() = default;
  BraveWalletProviderDelegate(const BraveWalletProviderDelegate&) = delete;
  BraveWalletProviderDelegate& operator=(const BraveWalletProviderDelegate&) =
      delete;
  virtual ~BraveWalletProviderDelegate() = default;

  virtual void ShowConnectToSiteUI() = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_
