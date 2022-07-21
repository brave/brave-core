/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_impl.h"

#include <utility>

#include "bat/ads/ad_content_info.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_info.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/ads/ad_events/ad_event_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/ads/inline_content_ad.h"
#include "bat/ads/internal/ads/new_tab_page_ad.h"
#include "bat/ads/internal/ads/notification_ad.h"
#include "bat/ads/internal/ads/promoted_content_ad.h"
#include "bat/ads/internal/ads/search_result_ad.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/browser/browser_manager.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/conversions/conversions.h"
#include "bat/ads/internal/covariates/covariate_manager.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/database/database_manager.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/diagnostics/diagnostic_manager.h"
#include "bat/ads/internal/features/features_util.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/internal/legacy_migration/client/legacy_client_migration.h"
#include "bat/ads/internal/legacy_migration/conversions/legacy_conversions_migration.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/prefs/pref_manager.h"
#include "bat/ads/internal/privacy/tokens/token_generator.h"
#include "bat/ads/internal/processors/behavioral/bandits/bandit_feedback_info.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/processors/behavioral/purchase_intent/purchase_intent_processor.h"
#include "bat/ads/internal/processors/contextual/text_classification/text_classification_processor.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/resources/resource_manager.h"
#include "bat/ads/internal/studies/studies_util.h"
#include "bat/ads/internal/tabs/tab_manager.h"
#include "bat/ads/internal/transfer/transfer.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_manager.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_manager.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/promoted_content_ad_info.h"
#include "bat/ads/statement_info.h"
#include "build/build_config.h"
#include "url/gurl.h"

namespace ads {

AdsImpl::AdsImpl(AdsClient* ads_client)
    : ads_client_helper_(std::make_unique<AdsClientHelper>(ads_client)) {
  browser_manager_ = std::make_unique<BrowserManager>();
  client_state_manager_ = std::make_unique<ClientStateManager>();
  confirmation_state_manager_ = std::make_unique<ConfirmationStateManager>();
  covariate_manager_ = std::make_unique<CovariateManager>();
  database_manager_ = std::make_unique<DatabaseManager>();
  diagnostic_manager_ = std::make_unique<DiagnosticManager>();
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

  epsilon_greedy_bandit_processor_ =
      std::make_unique<processor::EpsilonGreedyBandit>();
  purchase_intent_processor_ = std::make_unique<processor::PurchaseIntent>(
      purchase_intent_resource_.get());
  text_classification_processor_ =
      std::make_unique<processor::TextClassification>(
          text_classification_resource_.get());

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

  account_->AddObserver(this);
  conversions_->AddObserver(this);
  database_manager_->AddObserver(this);
  history_manager_->AddObserver(this);
  transfer_->AddObserver(this);
}

AdsImpl::~AdsImpl() {
  account_->RemoveObserver(this);
  conversions_->RemoveObserver(this);
  database_manager_->RemoveObserver(this);
  history_manager_->RemoveObserver(this);
  transfer_->RemoveObserver(this);
}

bool AdsImpl::IsInitialized() const {
  return is_initialized_;
}

void AdsImpl::Initialize(InitializeCallback callback) {
  BLOG(1, "Initializing ads");

  if (IsInitialized()) {
    BLOG(1, "Already initialized ads");
    FailedToInitialize(callback);
    return;
  }

  CreateOrOpenDatabase(callback);
}

void AdsImpl::Shutdown(ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(0, "Shutdown failed as not initialized");
    callback(/* success */ false);
    return;
  }

  NotificationAdManager::GetInstance()->CloseAndRemoveAll();

  callback(/* success */ true);
}

void AdsImpl::OnChangeLocale(const std::string& locale) {
  LocaleManager::GetInstance()->OnLocaleDidChange(locale);
}

void AdsImpl::OnPrefChanged(const std::string& path) {
  PrefManager::GetInstance()->OnPrefChanged(path);
}

void AdsImpl::OnHtmlLoaded(const int32_t tab_id,
                           const std::vector<GURL>& redirect_chain,
                           const std::string& html) {
  TabManager::GetInstance()->OnHtmlContentDidChange(tab_id, redirect_chain,
                                                    html);
}

void AdsImpl::OnTextLoaded(const int32_t tab_id,
                           const std::vector<GURL>& redirect_chain,
                           const std::string& text) {
  TabManager::GetInstance()->OnTextContentDidChange(tab_id, redirect_chain,
                                                    text);
}

void AdsImpl::OnUserGesture(const int32_t page_transition_type) {
  if (IsInitialized()) {
    UserActivityManager::GetInstance()->RecordEventForPageTransition(
        page_transition_type);
  }
}

void AdsImpl::OnIdle() {
  if (IsInitialized()) {
    IdleDetectionManager::GetInstance()->UserDidBecomeIdle();
  }
}

void AdsImpl::OnUnIdle(const base::TimeDelta idle_time, const bool was_locked) {
  if (IsInitialized()) {
    IdleDetectionManager::GetInstance()->UserDidBecomeActive(idle_time,
                                                             was_locked);
  }
}

void AdsImpl::OnBrowserDidEnterForeground() {
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();
}

void AdsImpl::OnBrowserDidEnterBackground() {
  BrowserManager::GetInstance()->OnBrowserDidEnterBackground();
}

void AdsImpl::OnMediaPlaying(const int32_t tab_id) {
  if (IsInitialized()) {
    TabManager::GetInstance()->OnMediaPlaying(tab_id);
  }
}

void AdsImpl::OnMediaStopped(const int32_t tab_id) {
  if (IsInitialized()) {
    TabManager::GetInstance()->OnMediaStopped(tab_id);
  }
}

void AdsImpl::OnTabUpdated(const int32_t tab_id,
                           const GURL& url,
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
  TabManager::GetInstance()->OnTabUpdated(tab_id, url, is_visible,
                                          is_incognito);
}

void AdsImpl::OnTabClosed(const int32_t tab_id) {
  if (IsInitialized()) {
    TabManager::GetInstance()->OnTabClosed(tab_id);
  }
}

void AdsImpl::OnWalletUpdated(const std::string& id, const std::string& seed) {
  account_->SetWallet(id, seed);
}

void AdsImpl::OnResourceComponentUpdated(const std::string& id) {
  ResourceManager::GetInstance()->UpdateResource(id);
}

bool AdsImpl::GetNotificationAd(const std::string& placement_id,
                                NotificationAdInfo* notification) {
  return NotificationAdManager::GetInstance()->GetForPlacementId(placement_id,
                                                                 notification);
}

void AdsImpl::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type) {
  notification_ad_->TriggerEvent(placement_id, event_type);
}

void AdsImpl::MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) {
  if (!IsInitialized()) {
    callback(/* success */ false, {});
    return;
  }

  new_tab_page_ad_->MaybeServe(callback);
}

void AdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) {
  new_tab_page_ad_->TriggerEvent(placement_id, creative_instance_id,
                                 event_type);
}

void AdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  promoted_content_ad_->TriggerEvent(placement_id, creative_instance_id,
                                     event_type);
}

void AdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  if (!IsInitialized()) {
    callback(/* success */ false, dimensions, {});
    return;
  }

  inline_content_ad_->MaybeServe(dimensions, callback);
}

void AdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  inline_content_ad_->TriggerEvent(placement_id, creative_instance_id,
                                   event_type);
}

void AdsImpl::TriggerSearchResultAdEvent(
    mojom::SearchResultAdPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type,
    TriggerSearchResultAdEventCallback callback) {
  if (!IsInitialized()) {
    callback(/* success */ false, ad_mojom->placement_id, event_type);
    return;
  }

  search_result_ad_->TriggerEvent(std::move(ad_mojom), event_type, callback);
}

void AdsImpl::PurgeOrphanedAdEventsForType(
    const mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  PurgeOrphanedAdEvents(ad_type, [ad_type, callback](const bool success) {
    if (!success) {
      BLOG(0, "Failed to purge orphaned ad events for " << ad_type);
      callback(/* success */ false);
      return;
    }

    BLOG(1, "Successfully purged orphaned ad events for " << ad_type);
    callback(/* success */ true);
  });
}

void AdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  ClientStateManager::GetInstance()->RemoveAllHistory();

  callback(/* success */ true);
}

HistoryInfo AdsImpl::GetHistory(const HistoryFilterType filter_type,
                                const HistorySortType sort_type,
                                const base::Time from_time,
                                const base::Time to_time) {
  if (!IsInitialized()) {
    return {};
  }

  return HistoryManager::GetInstance()->Get(filter_type, sort_type, from_time,
                                            to_time);
}

void AdsImpl::GetStatementOfAccounts(GetStatementOfAccountsCallback callback) {
  if (!IsInitialized()) {
    callback(/* success */ false, {});
    return;
  }

  account_->GetStatement(
      [callback](const bool success, const StatementInfo& statement) {
        callback(success, statement);
      });
}

void AdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  DiagnosticManager::GetInstance()->GetDiagnostics(callback);
}

AdContentLikeActionType AdsImpl::ToggleAdThumbUp(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);
  return HistoryManager::GetInstance()->LikeAd(ad_content);
}

AdContentLikeActionType AdsImpl::ToggleAdThumbDown(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);
  return HistoryManager::GetInstance()->DislikeAd(ad_content);
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

bool AdsImpl::ToggleFlaggedAd(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);
  return HistoryManager::GetInstance()->ToggleMarkAdAsInappropriate(ad_content);
}

bool AdsImpl::ToggleSavedAd(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);
  return HistoryManager::GetInstance()->ToggleSavedAd(ad_content);
}

///////////////////////////////////////////////////////////////////////////////

void AdsImpl::CreateOrOpenDatabase(InitializeCallback callback) {
  DatabaseManager::GetInstance()->CreateOrOpen([=](const bool success) {
    if (!success) {
      BLOG(0, "Failed to create or open database");
      FailedToInitialize(callback);
      return;
    }

    MigrateConversions(callback);
  });
}

void AdsImpl::MigrateConversions(InitializeCallback callback) {
  conversions::Migrate([=](const bool success) {
    if (!success) {
      FailedToInitialize(callback);
      return;
    }

    MigrateRewards(callback);
  });
}

void AdsImpl::MigrateRewards(InitializeCallback callback) {
  rewards::Migrate([=](const bool success) {
    if (!success) {
      FailedToInitialize(callback);
      return;
    }

    MigrateClientState(callback);
  });
}

void AdsImpl::MigrateClientState(InitializeCallback callback) {
  client::Migrate([=](const bool success) {
    if (!success) {
      FailedToInitialize(callback);
      return;
    }

    LoadClientState(callback);
  });
}

void AdsImpl::LoadClientState(InitializeCallback callback) {
  ClientStateManager::GetInstance()->Initialize([=](const bool success) {
    if (!success) {
      FailedToInitialize(callback);
      return;
    }

    LoadConfirmationState(callback);
  });
}

void AdsImpl::LoadConfirmationState(InitializeCallback callback) {
  ConfirmationStateManager::GetInstance()->Initialize([=](const bool success) {
    if (!success) {
      FailedToInitialize(callback);
      return;
    }

    LoadNotificationAdState(callback);
  });
}

void AdsImpl::LoadNotificationAdState(InitializeCallback callback) {
  NotificationAdManager::GetInstance()->Initialize([=](const bool success) {
    if (!success) {
      FailedToInitialize(callback);
      return;
    }

    SuccessfullyInitialized(callback);
  });
}

void AdsImpl::FailedToInitialize(InitializeCallback callback) {
  BLOG(1, "Failed to initialize ads");

  callback(/* success */ false);
}

void AdsImpl::SuccessfullyInitialized(InitializeCallback callback) {
  BLOG(1, "Successfully initialized ads");

  is_initialized_ = true;

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kInitializedAds);

  callback(/* success */ true);

  Start();
}

void AdsImpl::Start() {
  LogFeatures();

  LogActiveStudies();

#if BUILDFLAG(IS_ANDROID)
  // Notification ads do not sustain a reboot or update, so we should remove
  // orphaned notification ads
  NotificationAdManager::GetInstance()->RemoveAllAfterReboot();
  NotificationAdManager::GetInstance()->RemoveAllAfterUpdate();
#endif

  account_->Process();

  conversions_->Process();

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

void AdsImpl::OnDatabaseIsReady() {
  PurgeExpiredAdEvents();
}

void AdsImpl::OnDidLikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kUpvoted);
}

void AdsImpl::OnDidDislikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kDownvoted);
}

void AdsImpl::OnDidMarkAdAsInappropriate(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kFlagged);
}

void AdsImpl::OnDidSaveAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kSaved);
}

void AdsImpl::OnDidTransferAd(const AdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kTransferred);
}

}  // namespace ads
