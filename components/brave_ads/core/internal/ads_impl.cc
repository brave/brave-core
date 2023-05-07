/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_impl.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/ad_content_value_util.h"
#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/conversions/legacy_conversions_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/notifications/legacy_notification_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration.h"
#include "brave/components/brave_ads/core/internal/studies/studies_util.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

namespace brave_ads {

namespace {

void FailedToInitialize(InitializeCallback callback) {
  BLOG(0, "Failed to initialize ads");

  std::move(callback).Run(/*success*/ false);
}

}  // namespace

AdsImpl::AdsImpl(AdsClient* ads_client)
    : global_state_(ads_client),
      account_(&token_generator_),
      epsilon_greedy_bandit_resource_(catalog_),
      purchase_intent_processor_(purchase_intent_resource_),
      text_classification_processor_(text_classification_resource_),
      text_embedding_processor_(text_embedding_resource_),
      inline_content_ad_handler_(account_,
                                 transfer_,
                                 subdivision_targeting_,
                                 anti_targeting_resource_),
      new_tab_page_ad_handler_(account_,
                               transfer_,
                               subdivision_targeting_,
                               anti_targeting_resource_),
      notification_ad_handler_(account_,
                               transfer_,
                               subdivision_targeting_,
                               anti_targeting_resource_),
      promoted_content_ad_handler_(account_, transfer_),
      search_result_ad_handler_(account_, transfer_),
      user_reactions_(account_) {
  account_.AddObserver(this);
  conversions_.AddObserver(this);
  transfer_.AddObserver(this);
}

AdsImpl::~AdsImpl() {
  account_.RemoveObserver(this);
  conversions_.RemoveObserver(this);
  transfer_.RemoveObserver(this);
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

void AdsImpl::Initialize(InitializeCallback callback) {
  BLOG(1, "Initializing ads");

  if (IsInitialized()) {
    BLOG(1, "Already initialized ads");
    return FailedToInitialize(std::move(callback));
  }

  CreateOrOpenDatabase(std::move(callback));
}

void AdsImpl::Shutdown(ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(0, "Shutdown failed as not initialized");
    return std::move(callback).Run(/*success*/ false);
  }

  NotificationAdManager::GetInstance().CloseAll();

  NotificationAdManager::GetInstance().RemoveAll();

  std::move(callback).Run(/*success*/ true);
}

void AdsImpl::OnRewardsWalletDidChange(const std::string& payment_id,
                                       const std::string& recovery_seed) {
  account_.SetWallet(payment_id, recovery_seed);
}

absl::optional<NotificationAdInfo> AdsImpl::MaybeGetNotificationAd(
    const std::string& placement_id) {
  return NotificationAdManager::GetInstance().MaybeGetForPlacementId(
      placement_id);
}

void AdsImpl::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  notification_ad_handler_.TriggerEvent(placement_id, event_type);
}

void AdsImpl::MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*ad*/ absl::nullopt);
  }

  new_tab_page_ad_handler_.MaybeServe(std::move(callback));
}

void AdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  new_tab_page_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                        event_type);
}

void AdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  promoted_content_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                            event_type);
}

void AdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(dimensions, /*ad*/ absl::nullopt);
  }

  inline_content_ad_handler_.MaybeServe(dimensions, std::move(callback));
}

void AdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  inline_content_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                          event_type);
}

void AdsImpl::TriggerSearchResultAdEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (IsInitialized()) {
    search_result_ad_handler_.TriggerEvent(std::move(ad_mojom), event_type);
  }
}

void AdsImpl::PurgeOrphanedAdEventsForType(
    const mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  CHECK(mojom::IsKnownEnumValue(ad_type));

  PurgeOrphanedAdEvents(
      ad_type,
      base::BindOnce(
          [](const mojom::AdType ad_type,
             PurgeOrphanedAdEventsForTypeCallback callback,
             const bool success) {
            if (!success) {
              BLOG(0, "Failed to purge orphaned ad events for " << ad_type);
              return std::move(callback).Run(/*success*/ false);
            }

            RebuildAdEventHistoryFromDatabase();

            BLOG(1, "Successfully purged orphaned ad events for " << ad_type);
            std::move(callback).Run(/*success*/ true);
          },
          ad_type, std::move(callback)));
}

void AdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  ClientStateManager::GetInstance().RemoveAllHistory();

  std::move(callback).Run(/*success*/ true);
}

HistoryItemList AdsImpl::GetHistory(const HistoryFilterType filter_type,
                                    const HistorySortType sort_type,
                                    const base::Time from_time,
                                    const base::Time to_time) {
  if (!IsInitialized()) {
    return {};
  }

  return HistoryManager::Get(filter_type, sort_type, from_time, to_time);
}

void AdsImpl::GetStatementOfAccounts(GetStatementOfAccountsCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*statement*/ nullptr);
  }

  Account::GetStatement(std::move(callback));
}

void AdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  DiagnosticManager::GetInstance().GetDiagnostics(std::move(callback));
}

mojom::UserReactionType AdsImpl::ToggleLikeAd(const base::Value::Dict& value) {
  return HistoryManager::GetInstance().LikeAd(AdContentFromValue(value));
}

mojom::UserReactionType AdsImpl::ToggleDislikeAd(
    const base::Value::Dict& value) {
  return HistoryManager::GetInstance().DislikeAd(AdContentFromValue(value));
}

mojom::UserReactionType AdsImpl::ToggleLikeCategory(
    const std::string& category,
    const mojom::UserReactionType user_reaction_type) {
  return HistoryManager::GetInstance().LikeCategory(category,
                                                    user_reaction_type);
}

mojom::UserReactionType AdsImpl::ToggleDislikeCategory(
    const std::string& category,
    const mojom::UserReactionType user_reaction_type) {
  return HistoryManager::GetInstance().DislikeCategory(category,
                                                       user_reaction_type);
}

bool AdsImpl::ToggleSaveAd(const base::Value::Dict& value) {
  return HistoryManager::GetInstance().ToggleSaveAd(AdContentFromValue(value));
}

bool AdsImpl::ToggleMarkAdAsInappropriate(const base::Value::Dict& value) {
  return HistoryManager::GetInstance().ToggleMarkAdAsInappropriate(
      AdContentFromValue(value));
}

///////////////////////////////////////////////////////////////////////////////

void AdsImpl::CreateOrOpenDatabase(InitializeCallback callback) {
  DatabaseManager::GetInstance().CreateOrOpen(
      base::BindOnce(&AdsImpl::OnCreateOrOpenDatabase,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsImpl::OnCreateOrOpenDatabase(InitializeCallback callback,
                                     const bool success) {
  if (!success) {
    BLOG(0, "Failed to create or open database");
    return FailedToInitialize(std::move(callback));
  }

  PurgeExpiredAdEvents(base::BindOnce(&AdsImpl::OnPurgeExpiredAdEvents,
                                      weak_factory_.GetWeakPtr(),
                                      std::move(callback)));
}

void AdsImpl::OnPurgeExpiredAdEvents(InitializeCallback callback,
                                     const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  PurgeOrphanedAdEvents(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(&AdsImpl::OnPurgeOrphanedAdEvents,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsImpl::OnPurgeOrphanedAdEvents(InitializeCallback callback,
                                      const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  RebuildAdEventHistoryFromDatabase();

  conversions::Migrate(base::BindOnce(&AdsImpl::OnMigrateConversions,
                                      weak_factory_.GetWeakPtr(),
                                      std::move(callback)));
}

void AdsImpl::OnMigrateConversions(InitializeCallback callback,
                                   const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  rewards::Migrate(base::BindOnce(&AdsImpl::OnMigrateRewards,
                                  weak_factory_.GetWeakPtr(),
                                  std::move(callback)));
}

void AdsImpl::OnMigrateRewards(InitializeCallback callback,
                               const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  client::Migrate(base::BindOnce(&AdsImpl::OnMigrateClientState,
                                 weak_factory_.GetWeakPtr(),
                                 std::move(callback)));
}

void AdsImpl::OnMigrateClientState(InitializeCallback callback,
                                   const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  ClientStateManager::GetInstance().Initialize(
      base::BindOnce(&AdsImpl::OnLoadClientState, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void AdsImpl::OnLoadClientState(InitializeCallback callback,
                                const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  confirmations::Migrate(base::BindOnce(&AdsImpl::OnMigrateConfirmationState,
                                        weak_factory_.GetWeakPtr(),
                                        std::move(callback)));
}

void AdsImpl::OnMigrateConfirmationState(InitializeCallback callback,
                                         const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  ConfirmationStateManager::GetInstance().Initialize(
      account_.GetWallet(),
      base::BindOnce(&AdsImpl::OnLoadConfirmationState,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsImpl::OnLoadConfirmationState(InitializeCallback callback,
                                      const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  notifications::Migrate(base::BindOnce(&AdsImpl::OnMigrateNotificationState,
                                        weak_factory_.GetWeakPtr(),
                                        std::move(callback)));
}

void AdsImpl::OnMigrateNotificationState(InitializeCallback callback,
                                         const bool success) {
  if (!success) {
    return FailedToInitialize(std::move(callback));
  }

  BLOG(1, "Successfully initialized ads");

  is_initialized_ = true;

  AdsClientHelper::GetInstance()->BindPendingObservers();

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kInitializedAds);

  std::move(callback).Run(/*success*/ true);

  Start();
}

void AdsImpl::Start() {
  LogActiveStudies();

  account_.Process();

  conversions_.Process();

  subdivision_targeting_.MaybeAllow();
  subdivision_targeting_.MaybeFetch();

  catalog_.MaybeFetch();

  notification_ad_handler_.MaybeServeAtRegularIntervals();
}

void AdsImpl::OnStatementOfAccountsDidChange() {
  AdsClientHelper::GetInstance()->UpdateAdRewards();
}

void AdsImpl::OnConversion(
    const ConversionQueueItemInfo& conversion_queue_item) {
  account_.Deposit(conversion_queue_item.creative_instance_id,
                   conversion_queue_item.ad_type, conversion_queue_item.segment,
                   ConfirmationType::kConversion);
}

void AdsImpl::OnDidTransferAd(const AdInfo& ad) {
  account_.Deposit(ad.creative_instance_id, ad.type, ad.segment,
                   ConfirmationType::kTransferred);
}

}  // namespace brave_ads
