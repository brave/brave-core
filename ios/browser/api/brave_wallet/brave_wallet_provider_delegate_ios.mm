/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios+private.h"
#include "brave/ios/browser/api/url/url_origin_ios+private.h"
#include "net/base/mac/url_conversions.h"

namespace brave_wallet {

void BraveWalletProviderDelegateBridge::ShowPanel() {
  [bridge_ showPanel];
}

void BraveWalletProviderDelegateBridge::WalletInteractionDetected() {
  [bridge_ walletInteractionDetected];
}

url::Origin BraveWalletProviderDelegateBridge::GetOrigin() const {
  return url::Origin([[bridge_ getOrigin] underlyingOrigin]);
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
    mojom::CoinType type,
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
  [bridge_ getAllowedAccounts:static_cast<BraveWalletCoinType>(type)
      includeAccountsWhenLocked:include_accounts_when_locked
                     completion:completion];
}

void BraveWalletProviderDelegateBridge::RequestSolanaPermission(
    RequestSolanaPermissionCallback callback) {
  auto completion =
      [callback = std::make_shared<decltype(callback)>(std::move(callback))](
          NSString* _Nullable account, BraveWalletSolanaProviderError error,
          NSString* errorMessage) {
        if (!callback) {
          return;
        }
        std::move(*callback).Run(
            account == nil
                ? absl::nullopt
                : absl::optional<std::string>(base::SysNSStringToUTF8(account)),
            static_cast<brave_wallet::mojom::SolanaProviderError>(error),
            base::SysNSStringToUTF8(errorMessage));
      };
  [bridge_ requestSolanaPermission:completion];
}

void BraveWalletProviderDelegateBridge::IsSelectedAccountAllowed(
    mojom::CoinType type,
    IsSelectedAccountAllowedCallback callback) {
  auto completion =
      [callback = std::make_shared<decltype(callback)>(std::move(callback))](
          NSString* _Nullable account, bool allowed) {
        if (!callback) {
          return;
        }
        std::move(*callback).Run(
            account == nil
                ? absl::nullopt
                : absl::optional<std::string>(base::SysNSStringToUTF8(account)),
            allowed);
      };
  [bridge_ isSelectedAccountAllowed:static_cast<BraveWalletCoinType>(type)
                         completion:completion];
}

}  // namespace brave_wallet
