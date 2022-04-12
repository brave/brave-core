/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"

#include <utility>
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

namespace brave_wallet {

void BraveWalletServiceDelegate::IsExternalWalletInstalled(
    mojom::ExternalWalletType type,
    IsExternalWalletInstalledCallback callback) {
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::IsExternalWalletInitialized(
    mojom::ExternalWalletType type,
    IsExternalWalletInitializedCallback callback) {
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::GetImportInfoFromExternalWallet(
    mojom::ExternalWalletType type,
    const std::string& password,
    GetImportInfoCallback callback) {
  std::move(callback).Run(false, ImportInfo(), ImportError::kInternalError);
}

void BraveWalletServiceDelegate::AddEthereumPermission(
    const url::Origin& origin,
    const std::string& account,
    AddEthereumPermissionCallback callback) {
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::HasEthereumPermission(
    const url::Origin& origin,
    const std::string& account,
    HasEthereumPermissionCallback callback) {
  std::move(callback).Run(false, false);
}

void BraveWalletServiceDelegate::ResetEthereumPermission(
    const url::Origin& origin,
    const std::string& account,
    ResetEthereumPermissionCallback callback) {
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::GetActiveOrigin(
    GetActiveOriginCallback callback) {
  std::move(callback).Run(MakeOriginInfo(url::Origin()));
}

}  // namespace brave_wallet
