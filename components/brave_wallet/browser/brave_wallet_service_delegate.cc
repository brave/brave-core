/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"

#include <utility>
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

namespace brave_wallet {

absl::optional<ContentSettingsType>
BraveWalletServiceDelegate::CoinTypeToContentSettingsType(
    mojom::CoinType coin_type) {
  switch (coin_type) {
    case mojom::CoinType::ETH:
      return ContentSettingsType::BRAVE_ETHEREUM;
    case mojom::CoinType::SOL:
      return ContentSettingsType::BRAVE_SOLANA;
    default:
      return absl::nullopt;
  }
}

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

void BraveWalletServiceDelegate::AddPermission(mojom::CoinType coin,
                                               const url::Origin& origin,
                                               const std::string& account,
                                               AddPermissionCallback callback) {
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::HasPermission(mojom::CoinType coin,
                                               const url::Origin& origin,
                                               const std::string& account,
                                               HasPermissionCallback callback) {
  std::move(callback).Run(false, false);
}

void BraveWalletServiceDelegate::ResetPermission(
    mojom::CoinType coin,
    const url::Origin& origin,
    const std::string& account,
    ResetPermissionCallback callback) {
  std::move(callback).Run(false);
}

void BraveWalletServiceDelegate::GetActiveOrigin(
    GetActiveOriginCallback callback) {
  std::move(callback).Run(MakeOriginInfo(url::Origin()));
}

}  // namespace brave_wallet
