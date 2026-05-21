/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_initializer.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_state_manager.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

AdsInitializer::AdsInitializer() = default;

AdsInitializer::~AdsInitializer() = default;

void AdsInitializer::Initialize(mojom::WalletInfoPtr wallet,
                                ResultCallback callback) {
  CreateOrOpenDatabase(std::move(wallet), std::move(callback));
}

void AdsInitializer::CreateOrOpenDatabase(mojom::WalletInfoPtr wallet,
                                          ResultCallback callback) {
  DatabaseManager::GetInstance().CreateOrOpen(base::BindOnce(
      &AdsInitializer::CreateOrOpenDatabaseCallback, weak_factory_.GetWeakPtr(),
      std::move(wallet), std::move(callback)));
}

void AdsInitializer::CreateOrOpenDatabaseCallback(mojom::WalletInfoPtr wallet,
                                                  ResultCallback callback,
                                                  bool success) {
  if (!success) {
    BLOG(0, "Failed to create or open database");
    return std::move(callback).Run(/*success=*/false);
  }

  MigrateClientState(base::BindOnce(&AdsInitializer::MigrateClientStateCallback,
                                    weak_factory_.GetWeakPtr(),
                                    std::move(wallet), std::move(callback)));
}

void AdsInitializer::MigrateClientStateCallback(mojom::WalletInfoPtr wallet,
                                                ResultCallback callback,
                                                bool success) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false);
  }

  ClientStateManager::GetInstance().LoadState(base::BindOnce(
      &AdsInitializer::LoadClientStateCallback, weak_factory_.GetWeakPtr(),
      std::move(wallet), std::move(callback)));
}

void AdsInitializer::LoadClientStateCallback(mojom::WalletInfoPtr wallet,
                                             ResultCallback callback,
                                             bool success) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false);
  }

  std::optional<WalletInfo> wallet_info;
  if (wallet) {
    wallet_info = MaybeBuildWalletFromRecoverySeed(wallet.get());
    if (!wallet_info) {
      BLOG(0, "Invalid wallet");
      return std::move(callback).Run(/*success=*/false);
    }
  }

  MigrateConfirmationState(
      std::move(wallet_info),
      base::BindOnce(&AdsInitializer::MigrateConfirmationStateCallback,
                     weak_factory_.GetWeakPtr(), std::move(wallet),
                     std::move(callback)));
}

void AdsInitializer::MigrateConfirmationStateCallback(
    mojom::WalletInfoPtr wallet,
    ResultCallback callback,
    bool success) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false);
  }

  TokenStateManager::GetInstance().LoadState(base::BindOnce(
      &AdsInitializer::LoadConfirmationStateCallback,
      weak_factory_.GetWeakPtr(), std::move(wallet), std::move(callback)));
}

void AdsInitializer::LoadConfirmationStateCallback(mojom::WalletInfoPtr wallet,
                                                   ResultCallback callback,
                                                   bool success) {
  if (!success) {
    BLOG(0, "Failed to load confirmation state");
    return std::move(callback).Run(/*success=*/false);
  }

  SuccessfullyInitialized(std::move(wallet), std::move(callback));
}

void AdsInitializer::SuccessfullyInitialized(mojom::WalletInfoPtr wallet,
                                             ResultCallback callback) {
  std::optional<WalletInfo> wallet_info;
  if (wallet) {
    wallet_info = MaybeBuildWalletFromRecoverySeed(wallet.get());
    if (!wallet_info) {
      BLOG(0, "Invalid wallet");
      return std::move(callback).Run(/*success=*/false);
    }
  }
  GetAccount().SetWallet(std::move(wallet_info));

  GetAdsClient().NotifyPendingObservers();

  std::move(callback).Run(/*success=*/true);
}

}  // namespace brave_ads
