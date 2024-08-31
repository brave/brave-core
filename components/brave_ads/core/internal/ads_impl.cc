/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_impl.h"

#include <utility>

#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/database/database_maintenance.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/history/ad_history_value_util.h"

namespace brave_ads {

namespace {

void FailedToInitialize(InitializeCallback callback) {
  BLOG(0, "Failed to initialize ads");

  std::move(callback).Run(/*success=*/false);
}

}  // namespace

AdsImpl::AdsImpl(AdsClient* const ads_client,
                 std::unique_ptr<TokenGeneratorInterface> token_generator)
    : global_state_(ads_client, std::move(token_generator)),
      database_maintenance_(std::make_unique<database::Maintenance>()) {}

AdsImpl::~AdsImpl() = default;

void AdsImpl::AddBatAdsObserver(
    std::unique_ptr<AdsObserverInterface> observer) {
  // `AdsNotifierManager` takes ownership of `observer`.
  AdsNotifierManager::GetInstance().AddObserver(std::move(observer));
}

void AdsImpl::SetSysInfo(mojom::SysInfoPtr sys_info) {
  auto& sys_info_state = GlobalState::GetInstance()->SysInfo();
  sys_info_state.device_id = sys_info->device_id;
}

void AdsImpl::SetBuildChannel(mojom::BuildChannelInfoPtr build_channel) {
  auto& build_channel_state = GlobalState::GetInstance()->BuildChannel();
  build_channel_state.is_release = build_channel->is_release;
  build_channel_state.name = build_channel->name;
}

void AdsImpl::SetFlags(mojom::FlagsPtr flags) {
  auto& flags_state = GlobalState::GetInstance()->Flags();
  flags_state.should_debug = flags->should_debug;
  flags_state.did_override_from_command_line =
      flags->did_override_from_command_line;
  flags_state.environment_type = flags->environment_type;
}

void AdsImpl::Initialize(mojom::WalletInfoPtr mojom_wallet,
                         InitializeCallback callback) {
  BLOG(1, "Initializing ads");

  if (is_initialized_) {
    BLOG(1, "Already initialized ads");

    return FailedToInitialize(std::move(callback));
  }

  CreateOrOpenDatabase(std::move(mojom_wallet), std::move(callback));
}

void AdsImpl::Shutdown(ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(0, "Shutdown failed as not initialized");

    return std::move(callback).Run(/*success=*/false);
  }

  NotificationAdManager::GetInstance().RemoveAll(/*should_close=*/true);

  std::move(callback).Run(/*success=*/true);
}

void AdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*diagnostics=*/std::nullopt);
  }

  DiagnosticManager::GetInstance().GetDiagnostics(std::move(callback));
}

void AdsImpl::GetStatementOfAccounts(GetStatementOfAccountsCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*statement=*/nullptr);
  }

  GetAccount().GetStatement(std::move(callback));
}

void AdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(dimensions, /*ad=*/std::nullopt);
  }

  GetAdHandler().MaybeServeInlineContentAd(dimensions, std::move(callback));
}

void AdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  GetAdHandler().TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                             event_type, std::move(callback));
}

void AdsImpl::MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*ad=*/std::nullopt);
  }

  GetAdHandler().MaybeServeNewTabPageAd(std::move(callback));
}

void AdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  GetAdHandler().TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                          event_type, std::move(callback));
}

std::optional<NotificationAdInfo> AdsImpl::MaybeGetNotificationAd(
    const std::string& placement_id) {
  return NotificationAdManager::GetInstance().MaybeGetForPlacementId(
      placement_id);
}

void AdsImpl::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  GetAdHandler().TriggerNotificationAdEvent(placement_id, event_type,
                                            std::move(callback));
}

void AdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  GetAdHandler().TriggerPromotedContentAdEvent(
      placement_id, creative_instance_id, event_type, std::move(callback));
}

void AdsImpl::TriggerSearchResultAdEvent(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    const mojom::SearchResultAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  GetAdHandler().TriggerSearchResultAdEvent(std::move(mojom_creative_ad),
                                            event_type, std::move(callback));
}

void AdsImpl::PurgeOrphanedAdEventsForType(
    const mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  PurgeOrphanedAdEvents(
      ad_type,
      base::BindOnce(
          [](const mojom::AdType ad_type,
             PurgeOrphanedAdEventsForTypeCallback callback,
             const bool success) {
            if (!success) {
              BLOG(0, "Failed to purge orphaned ad events for " << ad_type);
            } else {
              BLOG(1, "Purged orphaned ad events for " << ad_type);
            }

            std::move(callback).Run(success);
          },
          ad_type, std::move(callback)));
}

void AdsImpl::GetAdHistory(const base::Time from_time,
                           const base::Time to_time,
                           GetAdHistoryForUICallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*ad_history*/ std::nullopt);
  }

  AdHistoryManager::GetForUI(from_time, to_time, std::move(callback));
}

void AdsImpl::ToggleLikeAd(const base::Value::Dict& value,
                           ToggleReactionCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  AdHistoryManager::GetInstance().LikeAd(AdHistoryItemFromValue(value),
                                         std::move(callback));
}

void AdsImpl::ToggleDislikeAd(const base::Value::Dict& value,
                              ToggleReactionCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  AdHistoryManager::GetInstance().DislikeAd(AdHistoryItemFromValue(value),
                                            std::move(callback));
}

void AdsImpl::ToggleLikeSegment(const base::Value::Dict& value,
                                ToggleReactionCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  AdHistoryManager::GetInstance().LikeSegment(AdHistoryItemFromValue(value),
                                              std::move(callback));
}

void AdsImpl::ToggleDislikeSegment(const base::Value::Dict& value,
                                   ToggleReactionCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  AdHistoryManager::GetInstance().DislikeSegment(AdHistoryItemFromValue(value),
                                                 std::move(callback));
}

void AdsImpl::ToggleSaveAd(const base::Value::Dict& value,
                           ToggleReactionCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  AdHistoryManager::GetInstance().ToggleSaveAd(AdHistoryItemFromValue(value),
                                               std::move(callback));
}

void AdsImpl::ToggleMarkAdAsInappropriate(const base::Value::Dict& value,
                                          ToggleReactionCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  AdHistoryManager::GetInstance().ToggleMarkAdAsInappropriate(
      AdHistoryItemFromValue(value), std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void AdsImpl::CreateOrOpenDatabase(mojom::WalletInfoPtr mojom_wallet,
                                   InitializeCallback callback) {
  DatabaseManager::GetInstance().CreateOrOpen(base::BindOnce(
      &AdsImpl::CreateOrOpenDatabaseCallback, weak_factory_.GetWeakPtr(),
      std::move(mojom_wallet), std::move(callback)));
}

void AdsImpl::CreateOrOpenDatabaseCallback(mojom::WalletInfoPtr mojom_wallet,
                                           InitializeCallback callback,
                                           const bool success) {
  if (!success) {
    BLOG(0, "Failed to create or open database");

    return FailedToInitialize(std::move(callback));
  }

  PurgeAllOrphanedAdEvents(base::BindOnce(
      &AdsImpl::PurgeAllOrphanedAdEventsCallback, weak_factory_.GetWeakPtr(),
      std::move(mojom_wallet), std::move(callback)));
}

void AdsImpl::SuccessfullyInitialized(mojom::WalletInfoPtr mojom_wallet,
                                      InitializeCallback callback) {
  BLOG(1, "Successfully initialized ads");

  is_initialized_ = true;

  if (mojom_wallet) {
    GetAccount().SetWallet(mojom_wallet->payment_id,
                           mojom_wallet->recovery_seed_base64);
  }

  GetAdsClient()->NotifyPendingObservers();

  std::move(callback).Run(/*success=*/true);
}

void AdsImpl::PurgeAllOrphanedAdEventsCallback(
    mojom::WalletInfoPtr mojom_wallet,
    InitializeCallback callback,
    const bool success) {
  if (!success) {
    BLOG(0, "Failed to purge all orphaned ad events");

    return FailedToInitialize(std::move(callback));
  }

  MigrateClientState(base::BindOnce(
      &AdsImpl::MigrateClientStateCallback, weak_factory_.GetWeakPtr(),
      std::move(mojom_wallet), std::move(callback)));
}

void AdsImpl::MigrateClientStateCallback(mojom::WalletInfoPtr mojom_wallet,
                                         InitializeCallback callback,
                                         const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  ClientStateManager::GetInstance().LoadState(base::BindOnce(
      &AdsImpl::LoadClientStateCallback, weak_factory_.GetWeakPtr(),
      std::move(mojom_wallet), std::move(callback)));
}

void AdsImpl::LoadClientStateCallback(mojom::WalletInfoPtr mojom_wallet,
                                      InitializeCallback callback,
                                      const bool success) {
  if (!success) {
    // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
    // potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Failed to load client state");
    base::debug::DumpWithoutCrashing();

    return FailedToInitialize(std::move(callback));
  }

  MigrateConfirmationState(base::BindOnce(
      &AdsImpl::MigrateConfirmationStateCallback, weak_factory_.GetWeakPtr(),
      std::move(mojom_wallet), std::move(callback)));
}

void AdsImpl::MigrateConfirmationStateCallback(
    mojom::WalletInfoPtr mojom_wallet,
    InitializeCallback callback,
    const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  std::optional<WalletInfo> wallet;
  if (mojom_wallet) {
    wallet = CreateWalletFromRecoverySeed(&*mojom_wallet);
    if (!wallet) {
      BLOG(0, "Invalid wallet");

      return FailedToInitialize(std::move(callback));
    }
  }

  ConfirmationStateManager::GetInstance().LoadState(
      wallet, base::BindOnce(&AdsImpl::LoadConfirmationStateCallback,
                             weak_factory_.GetWeakPtr(),
                             std::move(mojom_wallet), std::move(callback)));
}

void AdsImpl::LoadConfirmationStateCallback(mojom::WalletInfoPtr mojom_wallet,
                                            InitializeCallback callback,
                                            const bool success) {
  if (!success) {
    // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
    // potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Failed to load confirmation state");
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Failed to load confirmation state");

    return FailedToInitialize(std::move(callback));
  }

  SuccessfullyInitialized(std::move(mojom_wallet), std::move(callback));
}

}  // namespace brave_ads
