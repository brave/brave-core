/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_importer_delegate.h"

#include <utility>

namespace brave_wallet {

void BraveWalletImporterDelegate::IsCryptoWalletsInstalled(
    IsCryptoWalletsInstalledCallback callback) {
  std::move(callback).Run(false);
}
void BraveWalletImporterDelegate::IsMetaMaskInstalled(
    IsMetaMaskInstalledCallback callback) {
  std::move(callback).Run(false);
}
void BraveWalletImporterDelegate::ImportFromCryptoWallets(
    const std::string& password,
    const std::string& new_password,
    ImportFromCryptoWalletsCallback callback) {
  std::move(callback).Run(false);
}
void BraveWalletImporterDelegate::ImportFromMetaMask(
    const std::string& password,
    const std::string& new_password,
    ImportFromMetaMaskCallback callback) {
  std::move(callback).Run(false);
}

}  // namespace brave_wallet
