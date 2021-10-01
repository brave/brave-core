/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"

#include <utility>

namespace brave_wallet {

void BraveWalletServiceDelegate::IsCryptoWalletsInstalled(
    IsCryptoWalletsInstalledCallback callback) {
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::IsMetaMaskInstalled(
    IsMetaMaskInstalledCallback callback) {
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::GetImportInfoFromCryptoWallets(
    const std::string& password,
    GetImportInfoCallback callback) {
  std::move(callback).Run(false, ImportInfo());
}

void BraveWalletServiceDelegate::GetImportInfoFromMetaMask(
    const std::string& password,
    GetImportInfoCallback callback) {
  std::move(callback).Run(false, ImportInfo());
}

void BraveWalletServiceDelegate::HasEthereumPermission(
    const std::string& origin,
    const std::string& account,
    HasEthereumPermissionCallback callback) {
  std::move(callback).Run(false, false);
}

void BraveWalletServiceDelegate::ResetEthereumPermission(
    const std::string& origin,
    const std::string& account,
    ResetEthereumPermissionCallback callback) {
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::GetActiveOrigin(
    GetActiveOriginCallback callback) {
  std::move(callback).Run("");
}

}  // namespace brave_wallet
