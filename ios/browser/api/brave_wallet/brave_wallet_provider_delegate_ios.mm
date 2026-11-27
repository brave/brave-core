/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/base/apple/foundation_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios+private.h"
#include "brave/ios/browser/api/url/url_origin_ios+private.h"
#include "net/base/apple/url_conversions.h"

namespace brave_wallet {

namespace {

// Returns the subset of `accounts` whose permission identifier appears in
// `allowed_identifiers`, or an empty list if the bridge returned nil.
std::vector<mojom::AccountIdPtr> FilterAllowedAccounts(
    std::vector<mojom::AccountIdPtr> accounts,
    NSArray<NSString*>* _Nullable allowed_identifiers) {
  if (allowed_identifiers == nil) {
    return {};
  }
  auto identifiers = brave::ns_to_vector<std::string>(allowed_identifiers);
  std::vector<mojom::AccountIdPtr> allowed_accounts;
  for (auto& account : accounts) {
    if (std::ranges::contains(identifiers,
                              GetAccountPermissionIdentifier(account))) {
      allowed_accounts.push_back(std::move(account));
    }
  }
  return allowed_accounts;
}

void OnRequestPermissionsResult(
    std::vector<mojom::AccountIdPtr> queried_accounts,
    BraveWalletProviderDelegate::RequestPermissionsCallback callback,
    BraveWalletRequestPermissionsError error,
    NSArray<NSString*>* _Nullable results) {
  std::move(callback).Run(
      static_cast<mojom::RequestPermissionsError>(error),
      FilterAllowedAccounts(std::move(queried_accounts), results));
}

}  // namespace

bool BraveWalletProviderDelegateBridge::IsTabVisible() {
  return [bridge_ isTabVisible];
}

void BraveWalletProviderDelegateBridge::ShowPanel(const url::Origin& origin) {
  URLOriginIOS* origin_ios = [[URLOriginIOS alloc] initWithOrigin:origin];
  [bridge_ showPanelWithOrigin:origin_ios];
}

void BraveWalletProviderDelegateBridge::WalletInteractionDetected() {
  [bridge_ walletInteractionDetected];
}

void BraveWalletProviderDelegateBridge::ShowWalletBackup() {
  [bridge_ showWalletBackup];
}

void BraveWalletProviderDelegateBridge::UnlockWallet() {
  [bridge_ unlockWallet];
}

void BraveWalletProviderDelegateBridge::ShowAccountCreation(
    mojom::CoinType type,
    const url::Origin& origin) {
  URLOriginIOS* origin_ios = [[URLOriginIOS alloc] initWithOrigin:origin];
  [bridge_ showAccountCreation:static_cast<BraveWalletCoinType>(type)
                        origin:origin_ios];
}

void BraveWalletProviderDelegateBridge::RequestPermissions(
    const std::vector<mojom::AccountIdPtr>& accounts,
    const url::Origin& origin,
    RequestPermissionsCallback callback) {
  auto type = GetAccountsCoinType(accounts);
  auto completion = base::CallbackToBlock(base::BindOnce(
      &OnRequestPermissionsResult, CloneVector(accounts), std::move(callback)));
  URLOriginIOS* origin_ios = [[URLOriginIOS alloc] initWithOrigin:origin];
  NSArray* identifiers =
      brave::vector_to_ns(GetAccountPermissionIdentifiers(accounts));
  [bridge_ requestPermissions:static_cast<BraveWalletCoinType>(type)
                     accounts:identifiers
                       origin:origin_ios
                   completion:completion];
}

bool BraveWalletProviderDelegateBridge::IsAccountAllowed(
    const mojom::AccountIdPtr& account) {
  NSString* identifier =
      base::SysUTF8ToNSString(GetAccountPermissionIdentifier(account));
  return
      [bridge_ isAccountAllowed:static_cast<BraveWalletCoinType>(account->coin)
                        account:identifier];
}

std::vector<mojom::AccountIdPtr>
BraveWalletProviderDelegateBridge::GetAllowedAccounts(
    const std::vector<mojom::AccountIdPtr>& accounts) {
  auto type = GetAccountsCoinType(accounts);
  NSArray* identifiers =
      brave::vector_to_ns(GetAccountPermissionIdentifiers(accounts));
  NSArray<NSString*>* results =
      [bridge_ getAllowedAccounts:static_cast<BraveWalletCoinType>(type)
                         accounts:identifiers];
  return FilterAllowedAccounts(CloneVector(accounts), results);
}

bool BraveWalletProviderDelegateBridge::IsPermissionDenied(
    mojom::CoinType type) {
  return [bridge_ isPermissionDenied:static_cast<BraveWalletCoinType>(type)];
}

}  // namespace brave_wallet
