/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_PRIVATE_H_

#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"

@protocol BraveWalletProviderDelegate;

namespace brave_wallet {

class BraveWalletProviderDelegateBridge
    : public brave_wallet::BraveWalletProviderDelegate {
 public:
  explicit BraveWalletProviderDelegateBridge(
      id<BraveWalletProviderDelegate> bridge)
      : bridge_(bridge) {}

 private:
  __weak id<BraveWalletProviderDelegate> bridge_;

  void ShowPanel() override;
  GURL GetOrigin() const override;
  void RequestEthereumPermissions(
      RequestEthereumPermissionsCallback callback) override;
  void GetAllowedAccounts(bool include_accounts_when_locked,
                          GetAllowedAccountsCallback callback) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_PRIVATE_H_
