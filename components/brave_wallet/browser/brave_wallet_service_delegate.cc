/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/types/expected.h"

namespace brave_wallet {

void BraveWalletServiceDelegate::IsExternalWalletInstalled(
    mojom::ExternalWalletType type,
    IsExternalWalletInstalledCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::IsExternalWalletInitialized(
    mojom::ExternalWalletType type,
    IsExternalWalletInitializedCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::GetImportInfoFromExternalWallet(
    mojom::ExternalWalletType type,
    const std::string& password,
    GetImportInfoCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(base::unexpected(ImportError::kInternalError));
}

bool BraveWalletServiceDelegate::AddPermission(mojom::CoinType coin,
                                               const url::Origin& origin,
                                               const std::string& account) {
  NOTIMPLEMENTED();
  return false;
}

bool BraveWalletServiceDelegate::HasPermission(mojom::CoinType coin,
                                               const url::Origin& origin,
                                               const std::string& account) {
  NOTIMPLEMENTED();
  return false;
}

bool BraveWalletServiceDelegate::ResetPermission(mojom::CoinType coin,
                                                 const url::Origin& origin,
                                                 const std::string& account) {
  NOTIMPLEMENTED();
  return false;
}

bool BraveWalletServiceDelegate::IsPermissionDenied(mojom::CoinType coin,
                                                    const url::Origin& origin) {
  NOTIMPLEMENTED();
  return false;
}

std::optional<url::Origin> BraveWalletServiceDelegate::GetActiveOrigin() {
  NOTIMPLEMENTED();
  return std::nullopt;
}

void BraveWalletServiceDelegate::ClearWalletUIStoragePartition() {}

void BraveWalletServiceDelegate::GetWebSitesWithPermission(
    mojom::CoinType coin,
    GetWebSitesWithPermissionCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(std::vector<std::string>());
}

void BraveWalletServiceDelegate::ResetWebSitePermission(
    mojom::CoinType coin,
    const std::string& formed_website,
    ResetWebSitePermissionCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

}  // namespace brave_wallet
