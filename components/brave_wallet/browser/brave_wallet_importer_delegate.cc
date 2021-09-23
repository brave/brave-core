/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_importer_delegate.h"

#include <utility>

namespace brave_wallet {

void BraveWalletImporterDelegate::IsBraveCryptoWalletInstalled(
    IsBraveCryptoWalletInstalledCallback callback) {
  std::move(callback).Run(false);
}
void BraveWalletImporterDelegate::IsMetamaskInstalled(
    IsMetamaskInstalledCallback callback) {
  std::move(callback).Run(false);
}
void BraveWalletImporterDelegate::ImportFromBraveCryptoWallet(
    const std::string& password,
    const std::string& new_password,
    ImportFromBraveCryptoWalletCallback callback) {
  std::move(callback).Run(false);
}
void BraveWalletImporterDelegate::ImportFromMetamask(
    const std::string& password,
    const std::string& new_password,
    ImportFromMetamaskCallback callback) {
  std::move(callback).Run(false);
}

}  // namespace brave_wallet
