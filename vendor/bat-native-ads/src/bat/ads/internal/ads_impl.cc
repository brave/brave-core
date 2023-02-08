/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_impl.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "bat/ads/ad_content_info.h"
#include "bat/ads/ad_content_value_util.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/ads/inline_content_ad.h"
#include "bat/ads/internal/ads/new_tab_page_ad.h"
#include "bat/ads/internal/ads/notification_ad.h"
#include "bat/ads/internal/ads/promoted_content_ad.h"
#include "bat/ads/internal/ads/search_result_ad.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/browser/browser_manager.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/conversions/conversions.h"
#include "bat/ads/internal/covariates/covariate_manager.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager.h"
#include "bat/ads/internal/database/database_manager.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/diagnostics/diagnostic_manager.h"
#include "bat/ads/internal/features/features_util.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/internal/legacy_migration/client/legacy_client_migration.h"
#include "bat/ads/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"
#include "bat/ads/internal/legacy_migration/conversions/legacy_conversions_migration.h"
#include "bat/ads/internal/legacy_migration/notifications/legacy_notification_migration.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/prefs/pref_manager.h"
#include "bat/ads/internal/privacy/tokens/token_generator.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/processors/behavioral/purchase_intent/purchase_intent_processor.h"
#include "bat/ads/internal/processors/contextual/text_classification/text_classification_processor.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/resources/contextual/text_embedding/text_embedding_resource.h"
#include "bat/ads/internal/resources/resource_manager.h"
#include "bat/ads/internal/studies/studies_util.h"
#include "bat/ads/internal/tabs/tab_manager.h"
#include "bat/ads/internal/transfer/transfer.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_manager.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_manager.h"
#include "bat/ads/internal/user_interaction/user_reactions/user_reactions.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/public/interfaces/ads.mojom.h"  // IWYU pragma: keep
#include "url/gurl.h"

namespace ads {

namespace {

void FailedToInitialize(InitializeCallback callback) {
  BLOG(1, "Failed to initialize ads");

  std::move(callback).Run(/*success*/ false);
}

}  // namespace

AdsImpl::AdsImpl(AdsClient* ads_client)
    : ads_client_helper_(std::make_unique<AdsClientHelper>(ads_client)) {
  browser_manager_ = std::make_unique<BrowserManager>();
  client_state_manager_ = std::make_unique<ClientStateManager>();
  confirmation_state_manager_ = std::make_unique<ConfirmationStateManager>();
  covariate_manager_ = std::make_unique<CovariateManager>();
  database_manager_ = std::make_unique<DatabaseManager>();
  diagnostic_manager_ = std::make_unique<DiagnosticManager>();
  flag_manager_ = std::make_unique<FlagManager>();
  history_manager_ = std::make_unique<HistoryManager>();
  idle_detection_manager_ = std::make_unique<IdleDetectionManager>();
  locale_manager_ = std::make_unique<LocaleManager>();
  notification_ad_manager_ = std::make_unique<NotificationAdManager>();
  pref_manager_ = std::make_unique<PrefManager>();
  resource_manager_ = std::make_unique<ResourceManager>();
  tab_manager_ = std::make_unique<TabManager>();
  user_activity_manager_ = std::make_unique<UserActivityManager>();

  catalog_ = std::make_unique<Catalog>();

  token_generator_ = std::make_unique<privacy::TokenGenerator>();
  account_ = std::make_unique<Account>(token_generator_.get());

  transfer_ = std::make_unique<Transfer>();

  conversions_ = std::make_unique<Conversions>();

  subdivision_targeting_ = std::make_unique<geographic::SubdivisionTargeting>();

  anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
  epsilon_greedy_bandit_resource_ =
      std::make_unique<resource::EpsilonGreedyBandit>(catalog_.get());
  purchase_intent_resource_ = std::make_unique<resource::PurchaseIntent>();
  text_classification_resource_ =
      std::make_unique<resource::TextClassification>();
  text_embedding_resource_ = std::make_unique<resource::TextEmbedding>();

  epsilon_greedy_bandit_processor_ =
      std::make_unique<processor::EpsilonGreedyBandit>();
  purchase_intent_processor_ = std::make_unique<processor::PurchaseIntent>(
      purchase_intent_resource_.get());
  text_classification_processor_ =
      std::make_unique<processor::TextClassification>(
          text_classification_resource_.get());
  text_embedding_processor_ = std::make_unique<processor::TextEmbedding>(
      text_embedding_resource_.get());

  inline_content_ad_ = std::make_unique<InlineContentAd>(
      account_.get(), transfer_.get(), subdivision_targeting_.get(),
      anti_targeting_resource_.get());
  new_tab_page_ad_ = std::make_unique<NewTabPageAd>(
      account_.get(), transfer_.get(), subdivision_targeting_.get(),
      anti_targeting_resource_.get());
  notification_ad_ = std::make_unique<NotificationAd>(
      account_.get(), transfer_.get(), epsilon_greedy_bandit_processor_.get(),
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  promoted_content_ad_ =
      std::make_unique<PromotedContentAd>(account_.get(), transfer_.get());
  search_result_ad_ =
      std::make_unique<SearchResultAd>(account_.get(), transfer_.get());

  user_reactions_ = std::make_unique<UserReactions>(account_.get());

  account_->AddObserver(this);
  conversions_->AddObserver(this);
  transfer_->AddObserver(this);
}

AdsImpl::~AdsImpl() {
  account_->RemoveObserver(this);
  conversions_->RemoveObserver(this);
  transfer_->RemoveObserver(this);
}

bool AdsImpl::IsInitialized() const {
  return is_initialized_;
}

void AdsImpl::Initialize(InitializeCallback callback) {
  BLOG(1, "Initializing ads");

  if (IsInitialized()) {
    BLOG(1, "Already initialized ads");
    FailedToInitialize(std::move(callback));
    return;
  }

  CreateOrOpenDatabase(std::move(callback));
}

void AdsImpl::Shutdown(ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(0, "Shutdown failed as not initialized");
    std::move(callback).Run(/*success*/ false);
    return;
  }

  NotificationAdManager::GetInstance()->CloseAll();

  NotificationAdManager::GetInstance()->RemoveAll();

  std::move(callback).Run(/*success*/ true);
}

void AdsImpl::OnLocaleDidChange(const std::string& locale) {
  LocaleManager::GetInstance()->OnLocaleDidChange(locale);
}

void AdsImpl::OnPrefDidChange(const std::string& path) {
  if (IsInitialized()) {
    PrefManager::GetInstance()->OnPrefDidChange(path);
  }
}

void AdsImpl::OnTabHtmlContentDidChange(const int32_t tab_id,
                                        const std::vector<GURL>& redirect_chain,
                                        const std::string& html) {
  if (IsInitialized()) {
    TabManager::GetInstance()->OnHtmlContentDidChange(tab_id, redirect_chain,
                                                      html);
  }
}

void AdsImpl::OnTabTextContentDidChange(const int32_t tab_id,
                                        const std::vector<GURL>& redirect_chain,
                                        const std::string& text) {
  if (IsInitialized()) {
    TabManager::GetInstance()->OnTextContentDidChange(tab_id, redirect_chain,
                                                      text);
  }
}

void AdsImpl::TriggerUserGestureEvent(const int32_t page_transition_type) {
  if (IsInitialized()) {
    UserActivityManager::GetInstance()->RecordEventForPageTransition(
        page_transition_type);
  }
}

void AdsImpl::OnUserDidBecomeIdle() {
  if (IsInitialized()) {
    IdleDetectionManager::GetInstance()->UserDidBecomeIdle();
  }
}

void AdsImpl::OnUserDidBecomeActive(const base::TimeDelta idle_time,
                                    const bool screen_was_locked) {
  if (IsInitialized()) {
    IdleDetectionManager::GetInstance()->UserDidBecomeActive(idle_time,
                                                             screen_was_locked);
  }
}

void AdsImpl::OnBrowserDidEnterForeground() {
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();
}

void AdsImpl::OnBrowserDidEnterBackground() {
  BrowserManager::GetInstance()->OnBrowserDidEnterBackground();
}

void AdsImpl::OnTabDidStartPlayingMedia(const int32_t tab_id) {
  if (IsInitialized()) {
    TabManager::GetInstance()->OnDidStartPlayingMedia(tab_id);
  }
}

void AdsImpl::OnTabDidStopPlayingMedia(const int32_t tab_id) {
  if (IsInitialized()) {
    TabManager::GetInstance()->OnDidStopPlayingMedia(tab_id);
  }
}

void AdsImpl::OnTabDidChange(const int32_t tab_id,
                             const std::vector<GURL>& redirect_chain,
                             const bool is_active,
                             const bool is_browser_active,
                             const bool is_incognito) {
  if (!IsInitialized()) {
    return;
  }

  if (is_browser_active) {
    BrowserManager::GetInstance()->OnBrowserDidBecomeActive();
  } else {
    BrowserManager::GetInstance()->OnBrowserDidResignActive();
  }

  const bool is_visible = is_active && is_browser_active;
  TabManager::GetInstance()->OnDidChange(tab_id, redirect_chain, is_visible,
                                         is_incognito);
}

void AdsImpl::OnDidCloseTab(const int32_t tab_id) {
  if (IsInitialized()) {
    TabManager::GetInstance()->OnDidClose(tab_id);
  }
}

void AdsImpl::OnRewardsWalletDidChange(const std::string& payment_id,
                                       const std::string& recovery_seed) {
  account_->SetWallet(payment_id, recovery_seed);
}

void AdsImpl::OnDidUpdateResourceComponent(const std::string& id) {
  ResourceManager::GetInstance()->UpdateResource(id);
}

absl::optional<NotificationAdInfo> AdsImpl::MaybeGetNotificationAd(
    const std::string& placement_id) {
  return NotificationAdManager::GetInstance()->MaybeGetForPlacementId(
      placement_id);
}

void AdsImpl::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  notification_ad_->TriggerEvent(placement_id, event_type);
}

void AdsImpl::MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) {
  if (!IsInitialized()) {
    std::move(callback).Run(/*ads*/ absl::nullopt);
    return;
  }

  new_tab_page_ad_->MaybeServe(std::move(callback));
}

void AdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  new_tab_page_ad_->TriggerEvent(placement_id, creative_instance_id,
                                 event_type);
}

void AdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  promoted_content_ad_->TriggerEvent(placement_id, creative_instance_id,
                                     event_type);
}

void AdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  if (!IsInitialized()) {
    std::move(callback).Run(dimensions, absl::nullopt);
    return;
  }

  inline_content_ad_->MaybeServe(dimensions, std::move(callback));
}

void AdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  inline_content_ad_->TriggerEvent(placement_id, creative_instance_id,
                                   event_type);
}

void AdsImpl::TriggerSearchResultAdEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  if (!IsInitialized()) {
    return;
  }

  search_result_ad_->TriggerEvent(std::move(ad_mojom), event_type);
}

void AdsImpl::PurgeOrphanedAdEventsForType(
    const mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  DCHECK(ads::mojom::IsKnownEnumValue(ad_type));

  PurgeOrphanedAdEvents(
      ad_type,
      base::BindOnce(
          [](const mojom::AdType ad_type,
             PurgeOrphanedAdEventsForTypeCallback callback,
             const bool success) {
            if (!success) {
              BLOG(0, "Failed to purge orphaned ad events for " << ad_type);
              std::move(callback).Run(/*success*/ false);
              return;
            }

            RebuildAdEventHistoryFromDatabase();

            BLOG(1, "Successfully purged orphaned ad events for " << ad_type);
            std::move(callback).Run(/*success*/ true);
          },
          ad_type, std::move(callback)));
}

void AdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  ClientStateManager::GetInstance()->RemoveAllHistory();

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
    std::move(callback).Run(/*statement*/ nullptr);
    return;
  }

  Account::GetStatement(std::move(callback));
}

void AdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  DiagnosticManager::GetInstance()->GetDiagnostics(std::move(callback));
}

AdContentLikeActionType AdsImpl::ToggleAdThumbUp(base::Value::Dict value) {
  return HistoryManager::GetInstance()->LikeAd(AdContentFromValue(value));
}

AdContentLikeActionType AdsImpl::ToggleAdThumbDown(base::Value::Dict value) {
  return HistoryManager::GetInstance()->DislikeAd(AdContentFromValue(value));
}

CategoryContentOptActionType AdsImpl::ToggleAdOptIn(
    const std::string& category,
    const CategoryContentOptActionType& action_type) {
  return HistoryManager::GetInstance()->MarkToReceiveAdsForCategory(
      category, action_type);
}

CategoryContentOptActionType AdsImpl::ToggleAdOptOut(
    const std::string& category,
    const CategoryContentOptActionType& action_type) {
  return HistoryManager::GetInstance()->MarkToNoLongerReceiveAdsForCategory(
      category, action_type);
}

bool AdsImpl::ToggleFlaggedAd(base::Value::Dict value) {
  return HistoryManager::GetInstance()->ToggleMarkAdAsInappropriate(
      AdContentFromValue(value));
}

bool AdsImpl::ToggleSavedAd(base::Value::Dict value) {
  return HistoryManager::GetInstance()->ToggleSavedAd(
      AdContentFromValue(value));
}

///////////////////////////////////////////////////////////////////////////////

void AdsImpl::CreateOrOpenDatabase(InitializeCallback callback) {
  DatabaseManager::GetInstance()->CreateOrOpen(
      base::BindOnce(&AdsImpl::OnCreateOrOpenDatabase, base::Unretained(this),
                     std::move(callback)));
}

void AdsImpl::OnCreateOrOpenDatabase(InitializeCallback callback,
                                     const bool success) {
  if (!success) {
    BLOG(0, "Failed to create or open database");
    FailedToInitialize(std::move(callback));
    return;
  }

  PurgeExpiredAdEvents(base::BindOnce(&AdsImpl::OnPurgeExpiredAdEvents,
                                      base::Unretained(this),
                                      std::move(callback)));
}

void AdsImpl::OnPurgeExpiredAdEvents(InitializeCallback callback,
                                     const bool success) {
  if (!success) {
    FailedToInitialize(std::move(callback));
    return;
  }

  PurgeOrphanedAdEvents(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(&AdsImpl::OnPurgeOrphanedAdEvents, base::Unretained(this),
                     std::move(callback)));
}

void AdsImpl::OnPurgeOrphanedAdEvents(InitializeCallback callback,
                                      const bool success) {
  if (!success) {
    FailedToInitialize(std::move(callback));
    return;
  }

  RebuildAdEventHistoryFromDatabase();

  conversions::Migrate(base::BindOnce(&AdsImpl::OnMigrateConversions,
                                      base::Unretained(this),
                                      std::move(callback)));
}

void AdsImpl::OnMigrateConversions(InitializeCallback callback,
                                   const bool success) {
  if (!success) {
    FailedToInitialize(std::move(callback));
    return;
  }

  rewards::Migrate(base::BindOnce(&AdsImpl::OnMigrateRewards,
                                  base::Unretained(this), std::move(callback)));
}

void AdsImpl::OnMigrateRewards(InitializeCallback callback,
                               const bool success) {
  if (!success) {
    FailedToInitialize(std::move(callback));
    return;
  }

  client::Migrate(base::BindOnce(&AdsImpl::OnMigrateClientState,
                                 base::Unretained(this), std::move(callback)));
}

void AdsImpl::OnMigrateClientState(InitializeCallback callback,
                                   const bool success) {
  if (!success) {
    FailedToInitialize(std::move(callback));
    return;
  }

  ClientStateManager::GetInstance()->Initialize(
      base::BindOnce(&AdsImpl::OnLoadClientState, base::Unretained(this),
                     std::move(callback)));
}

void AdsImpl::OnLoadClientState(InitializeCallback callback,
                                const bool success) {
  if (!success) {
    FailedToInitialize(std::move(callback));
    return;
  }

  confirmations::Migrate(base::BindOnce(&AdsImpl::OnMigrateConfirmationState,
                                        base::Unretained(this),
                                        std::move(callback)));
}

void AdsImpl::OnMigrateConfirmationState(InitializeCallback callback,
                                         const bool success) {
  if (!success) {
    FailedToInitialize(std::move(callback));
    return;
  }

  ConfirmationStateManager::GetInstance()->Initialize(
      account_->GetWallet(),
      base::BindOnce(&AdsImpl::OnLoadConfirmationState, base::Unretained(this),
                     std::move(callback)));
}

void AdsImpl::OnLoadConfirmationState(InitializeCallback callback,
                                      const bool success) {
  if (!success) {
    FailedToInitialize(std::move(callback));
    return;
  }

  notifications::Migrate(base::BindOnce(&AdsImpl::OnMigrateNotificationState,
                                        base::Unretained(this),
                                        std::move(callback)));
}

void AdsImpl::OnMigrateNotificationState(InitializeCallback callback,
                                         const bool success) {
  if (!success) {
    FailedToInitialize(std::move(callback));
    return;
  }

  BLOG(1, "Successfully initialized ads");

  is_initialized_ = true;

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kInitializedAds);

  std::move(callback).Run(/*success*/ true);

  Start();
}

void AdsImpl::Start() {
  LogFeatures();

  LogActiveStudies();

  account_->Process();

  conversions_->Process();

  subdivision_targeting_->MaybeAllow();

  subdivision_targeting_->MaybeFetch();

  catalog_->MaybeFetch();

  notification_ad_->MaybeServeAtRegularIntervals();
}

void AdsImpl::OnStatementOfAccountsDidChange() {
  AdsClientHelper::GetInstance()->UpdateAdRewards();
}

void AdsImpl::OnConversion(
    const ConversionQueueItemInfo& conversion_queue_item) {
  account_->Deposit(conversion_queue_item.creative_instance_id,
                    conversion_queue_item.ad_type,
                    ConfirmationType::kConversion);
}

void AdsImpl::OnDidTransferAd(const AdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kTransferred);
}

}  // namespace ads
