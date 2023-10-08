/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_impl.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_cache_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/history/ad_content_info.h"
#include "brave/components/brave_ads/core/public/history/ad_content_value_util.h"
#include "brave/components/brave_ads/core/public/history/category_content_value_util.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

namespace brave_ads {

namespace {

void FailedToInitialize(InitializeCallback callback) {
  BLOG(0, "Failed to initialize ads");

  // TODO(https://github.com/brave/brave-browser/issues/32066): Remove migration
  // failure dumps.
  base::debug::DumpWithoutCrashing();

  std::move(callback).Run(/*success=*/false);
}

}  // namespace

AdsImpl::AdsImpl(AdsClient* ads_client)
    : global_state_(ads_client),
      account_(&token_generator_),
      ad_handler_(account_),
      user_reactions_(account_) {
  account_.AddObserver(this);
}

AdsImpl::~AdsImpl() {
  account_.RemoveObserver(this);
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

void AdsImpl::Initialize(mojom::WalletInfoPtr wallet,
                         InitializeCallback callback) {
  BLOG(1, "Initializing ads");

  if (is_initialized_) {
    BLOG(1, "Already initialized ads");
    return FailedToInitialize(std::move(callback));
  }

  CreateOrOpenDatabase(std::move(wallet), std::move(callback));
}

void AdsImpl::Shutdown(ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(0, "Shutdown failed as not initialized");
    return std::move(callback).Run(/*success=*/false);
  }

  NotificationAdManager::GetInstance().RemoveAll(/*should_close=*/true);

  std::move(callback).Run(/*success=*/true);
}

absl::optional<NotificationAdInfo> AdsImpl::MaybeGetNotificationAd(
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

  ad_handler_.TriggerNotificationAdEvent(placement_id, event_type,
                                         std::move(callback));
}

void AdsImpl::MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*ad=*/absl::nullopt);
  }

  ad_handler_.MaybeServeNewTabPageAd(std::move(callback));
}

void AdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  ad_handler_.TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                       event_type, std::move(callback));
}

void AdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  ad_handler_.TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                            event_type, std::move(callback));
}

void AdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(dimensions, /*ad=*/absl::nullopt);
  }

  ad_handler_.MaybeServeInlineContentAd(dimensions, std::move(callback));
}

void AdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  ad_handler_.TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                          event_type, std::move(callback));
}

void AdsImpl::TriggerSearchResultAdEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*success=*/false);
  }

  ad_handler_.TriggerSearchResultAdEvent(std::move(ad_mojom), event_type,
                                         std::move(callback));
}

void AdsImpl::PurgeOrphanedAdEventsForType(
    const mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  CHECK(mojom::IsKnownEnumValue(ad_type));

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
              return std::move(callback).Run(/*success=*/false);
            }

            RebuildAdEventCache();

            BLOG(1, "Successfully purged orphaned ad events for " << ad_type);
            std::move(callback).Run(/*success=*/true);
          },
          ad_type, std::move(callback)));
}

HistoryItemList AdsImpl::GetHistory(const HistoryFilterType filter_type,
                                    const HistorySortType sort_type,
                                    const base::Time from_time,
                                    const base::Time to_time) {
  return is_initialized_
             ? HistoryManager::Get(filter_type, sort_type, from_time, to_time)
             : HistoryItemList{};
}

void AdsImpl::GetStatementOfAccounts(GetStatementOfAccountsCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*statement=*/nullptr);
  }

  Account::GetStatement(std::move(callback));
}

void AdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  if (!is_initialized_) {
    return std::move(callback).Run(/*diagnostics=*/absl::nullopt);
  }

  DiagnosticManager::GetInstance().GetDiagnostics(std::move(callback));
}

mojom::UserReactionType AdsImpl::ToggleLikeAd(const base::Value::Dict& value) {
  return is_initialized_
             ? HistoryManager::GetInstance().LikeAd(AdContentFromValue(value))
             : mojom::UserReactionType::kNeutral;
}

mojom::UserReactionType AdsImpl::ToggleDislikeAd(
    const base::Value::Dict& value) {
  return is_initialized_ ? HistoryManager::GetInstance().DislikeAd(
                               AdContentFromValue(value))
                         : mojom::UserReactionType::kNeutral;
}

mojom::UserReactionType AdsImpl::ToggleLikeCategory(
    const base::Value::Dict& value) {
  return is_initialized_ ? HistoryManager::GetInstance().LikeCategory(
                               CategoryContentFromValue(value))
                         : mojom::UserReactionType::kNeutral;
}

mojom::UserReactionType AdsImpl::ToggleDislikeCategory(
    const base::Value::Dict& value) {
  return is_initialized_ ? HistoryManager::GetInstance().DislikeCategory(
                               CategoryContentFromValue(value))
                         : mojom::UserReactionType::kNeutral;
}

bool AdsImpl::ToggleSaveAd(const base::Value::Dict& value) {
  return is_initialized_ ? HistoryManager::GetInstance().ToggleSaveAd(
                               AdContentFromValue(value))
                         : false;
}

bool AdsImpl::ToggleMarkAdAsInappropriate(const base::Value::Dict& value) {
  return is_initialized_
             ? HistoryManager::GetInstance().ToggleMarkAdAsInappropriate(
                   AdContentFromValue(value))
             : false;
}

///////////////////////////////////////////////////////////////////////////////

void AdsImpl::CreateOrOpenDatabase(mojom::WalletInfoPtr wallet,
                                   InitializeCallback callback) {
  DatabaseManager::GetInstance().CreateOrOpen(base::BindOnce(
      &AdsImpl::CreateOrOpenDatabaseCallback, weak_factory_.GetWeakPtr(),
      std::move(wallet), std::move(callback)));
}

void AdsImpl::CreateOrOpenDatabaseCallback(mojom::WalletInfoPtr wallet,
                                           InitializeCallback callback,
                                           const bool success) {
  if (!success) {
    BLOG(0, "Failed to create or open database");
    return FailedToInitialize(std::move(callback));
  }

  PurgeExpiredAdEvents(base::BindOnce(&AdsImpl::PurgeExpiredAdEventsCallback,
                                      weak_factory_.GetWeakPtr(),
                                      std::move(wallet), std::move(callback)));
}

void AdsImpl::PurgeExpiredAdEventsCallback(mojom::WalletInfoPtr wallet,
                                           InitializeCallback callback,
                                           const bool success) {
  if (!success) {
    BLOG(0, "Failed to purge expired ad events");
    return FailedToInitialize(std::move(callback));
  }

  PurgeAllOrphanedAdEvents(base::BindOnce(
      &AdsImpl::PurgeOrphanedAdEventsCallback, weak_factory_.GetWeakPtr(),
      std::move(wallet), std::move(callback)));
}

void AdsImpl::PurgeOrphanedAdEventsCallback(mojom::WalletInfoPtr wallet,
                                            InitializeCallback callback,
                                            const bool success) {
  if (!success) {
    BLOG(0, "Failed to purge orphaned ad events");
    return FailedToInitialize(std::move(callback));
  }

  RebuildAdEventCache();

  rewards::Migrate(base::BindOnce(&AdsImpl::MigrateRewardsStateCallback,
                                  weak_factory_.GetWeakPtr(), std::move(wallet),
                                  std::move(callback)));
}

void AdsImpl::MigrateRewardsStateCallback(mojom::WalletInfoPtr wallet,
                                          InitializeCallback callback,
                                          const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  MigrateClientState(base::BindOnce(&AdsImpl::MigrateClientStateCallback,
                                    weak_factory_.GetWeakPtr(),
                                    std::move(wallet), std::move(callback)));
}

void AdsImpl::MigrateClientStateCallback(mojom::WalletInfoPtr wallet,
                                         InitializeCallback callback,
                                         const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  ClientStateManager::GetInstance().LoadState(base::BindOnce(
      &AdsImpl::LoadClientStateCallback, weak_factory_.GetWeakPtr(),
      std::move(wallet), std::move(callback)));
}

void AdsImpl::LoadClientStateCallback(mojom::WalletInfoPtr wallet,
                                      InitializeCallback callback,
                                      const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  MigrateConfirmationState(base::BindOnce(
      &AdsImpl::MigrateConfirmationStateCallback, weak_factory_.GetWeakPtr(),
      std::move(wallet), std::move(callback)));
}

void AdsImpl::MigrateConfirmationStateCallback(mojom::WalletInfoPtr wallet,
                                               InitializeCallback callback,
                                               const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  absl::optional<WalletInfo> new_wallet;
  if (wallet) {
    new_wallet = ToWallet(wallet->payment_id, wallet->recovery_seed);
    if (!new_wallet) {
      // TODO(https://github.com/brave/brave-browser/issues/32066): Remove
      // migration failure dumps.
      base::debug::DumpWithoutCrashing();

      BLOG(0, "Invalid wallet");
      return FailedToInitialize(std::move(callback));
    }
  }

  ConfirmationStateManager::GetInstance().LoadState(
      new_wallet, base::BindOnce(&AdsImpl::LoadConfirmationStateCallback,
                                 weak_factory_.GetWeakPtr(), std::move(wallet),
                                 std::move(callback)));
}

void AdsImpl::LoadConfirmationStateCallback(mojom::WalletInfoPtr wallet,
                                            InitializeCallback callback,
                                            const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  SuccessfullyInitialized(std::move(wallet), std::move(callback));
}

void AdsImpl::SuccessfullyInitialized(mojom::WalletInfoPtr wallet,
                                      InitializeCallback callback) {
  BLOG(1, "Successfully initialized ads");

  is_initialized_ = true;

  if (wallet) {
    account_.SetWallet(wallet->payment_id, wallet->recovery_seed);
  }

  NotifyPendingAdsClientObservers();

  std::move(callback).Run(/*success=*/true);
}

void AdsImpl::OnStatementOfAccountsDidChange() {
  // TODO(https://github.com/brave/brave-browser/issues/28726): Decouple.
  UpdateAdRewards();
}

}  // namespace brave_ads
