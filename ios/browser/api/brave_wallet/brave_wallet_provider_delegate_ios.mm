/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios+private.h"
#include "net/base/mac/url_conversions.h"

namespace brave_wallet {

void BraveWalletProviderDelegateBridge::ShowPanel() {
  [bridge_ showPanel];
}

GURL BraveWalletProviderDelegateBridge::GetOrigin() const {
  return net::GURLWithNSURL([bridge_ getOrigin]);
}

void BraveWalletProviderDelegateBridge::RequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback) {
  auto completion =
      [callback = std::make_shared<decltype(callback)>(std::move(callback))](
          NSArray<NSString*>* results, BraveWalletProviderError error,
          NSString* errorMessage) {
        // const std::vector<std::string>& results, mojom::ProviderError error,
        // const std::string& error_message
        if (!callback) {
          return;
        }
        std::vector<std::string> v;
        for (NSString* result in results) {
          v.push_back(base::SysNSStringToUTF8(result));
        }
        std::move(*callback).Run(
            v, static_cast<brave_wallet::mojom::ProviderError>(error),
            base::SysNSStringToUTF8(errorMessage));
      };
  [bridge_ requestEthereumPermissions:completion];
}

void BraveWalletProviderDelegateBridge::GetAllowedAccounts(
    bool include_accounts_when_locked,
    GetAllowedAccountsCallback callback) {
  auto completion =
      [callback = std::make_shared<decltype(callback)>(std::move(callback))](
          NSArray<NSString*>* results, BraveWalletProviderError error,
          NSString* errorMessage) {
        // const std::vector<std::string>& results, mojom::ProviderError error,
        // const std::string& error_message
        if (!callback) {
          return;
        }
        std::vector<std::string> v;
        for (NSString* result in results) {
          v.push_back(base::SysNSStringToUTF8(result));
        }
        std::move(*callback).Run(
            v, static_cast<brave_wallet::mojom::ProviderError>(error),
            base::SysNSStringToUTF8(errorMessage));
      };
  [bridge_ getAllowedAccounts:include_accounts_when_locked
                   completion:completion];
}

}  // namespace brave_wallet
