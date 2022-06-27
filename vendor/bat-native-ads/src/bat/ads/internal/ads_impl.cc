/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_impl.h"

#include "bat/ads/ad_content_info.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_info.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/ad_events/ad_event_util.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ad_events/inline_content_ads/inline_content_ad.h"
#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad.h"
#include "bat/ads/internal/ad_events/notification_ads/notification_ad.h"
#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad.h"
#include "bat/ads/internal/ad_events/search_result_ads/search_result_ad.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/platform/platform_helper.h"
#include "bat/ads/internal/browser/browser_manager.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/catalog/catalog_info.h"
#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/conversions/conversions.h"
#include "bat/ads/internal/covariates/covariate_manager.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/database/database_manager.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/diagnostics/diagnostic_manager.h"
#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_util.h"
#include "bat/ads/internal/features/features_util.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/history/history.h"
#include "bat/ads/internal/legacy_migration/conversions/legacy_conversions_migration.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/prefs/pref_manager.h"
#include "bat/ads/internal/privacy/tokens/token_generator.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/processors/behavioral/purchase_intent/purchase_intent_processor.h"
#include "bat/ads/internal/processors/contextual/text_classification/text_classification_processor.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/resources/resource_manager.h"
#include "bat/ads/internal/serving/inline_content_ad_serving.h"
#include "bat/ads/internal/serving/new_tab_page_ad_serving.h"
#include "bat/ads/internal/serving/notification_ad_serving.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/internal/studies/studies_util.h"
#include "bat/ads/internal/tabs/tab_manager.h"
#include "bat/ads/internal/transfer/transfer.h"
#include "bat/ads/internal/user_interaction/browsing/user_activity_manager.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_time.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/pref_names.h"
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
  locale_manager_ = std::make_unique<LocaleManager>();
  notification_ad_manager_ = std::make_unique<NotificationAdManager>();
  pref_manager_ = std::make_unique<PrefManager>();
  resource_manager_ = std::make_unique<ResourceManager>();
  tab_manager_ = std::make_unique<TabManager>();
  user_activity_manager_ = std::make_unique<UserActivityManager>();

  anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
  epsilon_greedy_bandit_resource_ =
      std::make_unique<resource::EpsilonGreedyBandit>();
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

  token_generator_ = std::make_unique<privacy::TokenGenerator>();
  account_ = std::make_unique<Account>(token_generator_.get());
  account_->AddObserver(this);

  catalog_ = std::make_unique<Catalog>();
  catalog_->AddObserver(this);

  subdivision_targeting_ = std::make_unique<geographic::SubdivisionTargeting>();

  inline_content_ad_ = std::make_unique<InlineContentAd>();
  inline_content_ad_->AddObserver(this);
  inline_content_ad_serving_ = std::make_unique<inline_content_ads::Serving>(
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  inline_content_ad_serving_->AddObserver(this);

  new_tab_page_ad_ = std::make_unique<NewTabPageAd>();
  new_tab_page_ad_->AddObserver(this);
  new_tab_page_ad_serving_ = std::make_unique<new_tab_page_ads::Serving>(
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  new_tab_page_ad_serving_->AddObserver(this);

  notification_ad_ = std::make_unique<NotificationAd>();
  notification_ad_->AddObserver(this);
  notification_ad_serving_ = std::make_unique<notification_ads::Serving>(
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  notification_ad_serving_->AddObserver(this);

  promoted_content_ad_ = std::make_unique<PromotedContentAd>();
  promoted_content_ad_->AddObserver(this);

  search_result_ad_ = std::make_unique<SearchResultAd>();
  search_result_ad_->AddObserver(this);

  conversions_ = std::make_unique<Conversions>();
  conversions_->AddObserver(this);

  transfer_ = std::make_unique<Transfer>();
  transfer_->AddObserver(this);
}

AdsImpl::~AdsImpl() {
  account_->RemoveObserver(this);

  catalog_->RemoveObserver(this);

  inline_content_ad_serving_->RemoveObserver(this);
  inline_content_ad_->RemoveObserver(this);

  new_tab_page_ad_serving_->RemoveObserver(this);
  new_tab_page_ad_->RemoveObserver(this);

  notification_ad_serving_->RemoveObserver(this);
  notification_ad_->RemoveObserver(this);

  promoted_content_ad_->RemoveObserver(this);

  search_result_ad_->RemoveObserver(this);

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
    callback(/* success */ false);
    return;
  }

  InitializeDatabase(callback);
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

void AdsImpl::ChangeLocale(const std::string& locale) {
  LocaleManager::GetInstance()->OnLocaleDidChange(locale);
}

void AdsImpl::OnPrefChanged(const std::string& path) {
  if (path == prefs::kEnabled) {
    MaybeServeNotificationAdsAtRegularIntervals();
  }

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
  if (!IsInitialized()) {
    return;
  }

  UserActivityManager::GetInstance()->RecordEventForPageTransition(
      page_transition_type);
}

void AdsImpl::OnIdle() {
  BLOG(1, "Browser state changed to idle");
}

void AdsImpl::OnUnIdle(const int idle_time, const bool was_locked) {
  if (!IsInitialized()) {
    return;
  }

  SetLastUnIdleTimeDiagnosticEntry();

  MaybeUpdateIdleTimeThreshold();

  BLOG(1, "Browser state changed to unidle after " << base::Seconds(idle_time));

  if (HasCatalogExpired()) {
    catalog_->MaybeFetch();
  }

  if (!ShouldRewardUser()) {
    return;
  }

  if (WasLocked(was_locked)) {
    BLOG(1, "Notification ad not served: Screen was locked");
    return;
  }

  if (HasExceededMaximumIdleTime(idle_time)) {
    BLOG(1, "Notification ad not served: Exceeded maximum idle time");
    return;
  }

  MaybeServeNotificationAd();
}

void AdsImpl::OnBrowserDidEnterForeground() {
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();

  MaybeServeNotificationAdsAtRegularIntervals();
}

void AdsImpl::OnBrowserDidEnterBackground() {
  BrowserManager::GetInstance()->OnBrowserDidEnterBackground();

  MaybeServeNotificationAdsAtRegularIntervals();
}

void AdsImpl::OnMediaPlaying(const int32_t tab_id) {
  if (!IsInitialized()) {
    return;
  }

  TabManager::GetInstance()->OnMediaPlaying(tab_id);
}

void AdsImpl::OnMediaStopped(const int32_t tab_id) {
  if (!IsInitialized()) {
    return;
  }

  TabManager::GetInstance()->OnMediaStopped(tab_id);
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
  if (!IsInitialized()) {
    return;
  }

  TabManager::GetInstance()->OnTabClosed(tab_id);
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
  notification_ad_->FireEvent(placement_id, event_type);
}

void AdsImpl::GetNewTabPageAd(GetNewTabPageAdCallback callback) {
  if (!IsInitialized()) {
    callback(/* success */ false, {});
    return;
  }

  new_tab_page_ad_serving_->MaybeServeAd(
      [=](const bool success, const NewTabPageAdInfo& ad) {
        callback(success, ad);
      });
}

void AdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) {
  new_tab_page_ad_->FireEvent(placement_id, creative_instance_id, event_type);
}

void AdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  promoted_content_ad_->FireEvent(placement_id, creative_instance_id,
                                  event_type);
}

void AdsImpl::GetInlineContentAd(const std::string& dimensions,
                                 GetInlineContentAdCallback callback) {
  if (!IsInitialized()) {
    callback(/* success */ false, dimensions, {});
    return;
  }

  inline_content_ad_serving_->MaybeServeAd(
      dimensions, [=](const bool success, const std::string& dimensions,
                      const InlineContentAdInfo& ad) {
        callback(success, dimensions, ad);
      });
}

void AdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  inline_content_ad_->FireEvent(placement_id, creative_instance_id, event_type);
}

void AdsImpl::TriggerSearchResultAdEvent(
    mojom::SearchResultAdPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type,
    TriggerSearchResultAdEventCallback callback) {
  if (!IsInitialized()) {
    callback(/* success */ false, ad_mojom->placement_id, event_type);
    return;
  }

  search_result_ad_->FireEvent(
      ad_mojom, event_type,
      [=](const bool success, const std::string& placement_id,
          const mojom::SearchResultAdEventType event_type) {
        callback(success, placement_id, event_type);
      });
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

  return history::Get(filter_type, sort_type, from_time, to_time);
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

  const AdContentLikeActionType like_action_type =
      ClientStateManager::GetInstance()->ToggleAdThumbUp(ad_content);
  if (like_action_type == AdContentLikeActionType::kThumbsUp) {
    account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                      ConfirmationType::kUpvoted);
  }

  return like_action_type;
}

AdContentLikeActionType AdsImpl::ToggleAdThumbDown(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);

  const AdContentLikeActionType like_action_type =
      ClientStateManager::GetInstance()->ToggleAdThumbDown(ad_content);
  if (like_action_type == AdContentLikeActionType::kThumbsDown) {
    account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                      ConfirmationType::kDownvoted);
  }

  return like_action_type;
}

CategoryContentOptActionType AdsImpl::ToggleAdOptIn(
    const std::string& category,
    const CategoryContentOptActionType& action) {
  return ClientStateManager::GetInstance()->ToggleAdOptIn(category, action);
}

CategoryContentOptActionType AdsImpl::ToggleAdOptOut(
    const std::string& category,
    const CategoryContentOptActionType& action) {
  return ClientStateManager::GetInstance()->ToggleAdOptOut(category, action);
}

bool AdsImpl::ToggleSavedAd(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);

  const bool is_saved =
      ClientStateManager::GetInstance()->ToggleSavedAd(ad_content);
  if (is_saved) {
    account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                      ConfirmationType::kSaved);
  }

  return is_saved;
}

bool AdsImpl::ToggleFlaggedAd(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);

  const bool is_flagged =
      ClientStateManager::GetInstance()->ToggleFlaggedAd(ad_content);
  if (is_flagged) {
    account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                      ConfirmationType::kFlagged);
  }

  return is_flagged;
}

///////////////////////////////////////////////////////////////////////////////

void AdsImpl::InitializeDatabase(InitializeCallback callback) {
  DatabaseManager::GetInstance()->CreateOrOpen([=](const bool success) {
    if (!success) {
      BLOG(0, "Failed to create or open database");
      callback(/* success */ false);
      return;
    }

    RebuildAdEventsFromDatabase();

    MigrateConversions(callback);
  });
}

void AdsImpl::MigrateConversions(InitializeCallback callback) {
  conversions::Migrate([=](const bool success) {
    if (!success) {
      callback(/* success */ false);
      return;
    }

    MigrateRewards(callback);
  });
}

void AdsImpl::MigrateRewards(InitializeCallback callback) {
  rewards::Migrate([=](const bool success) {
    if (!success) {
      callback(/* success */ false);
      return;
    }

    LoadClientState(callback);
  });
}

void AdsImpl::LoadClientState(InitializeCallback callback) {
  ClientStateManager::GetInstance()->Initialize([=](const bool success) {
    if (!success) {
      callback(/* success */ false);
      return;
    }

    LoadConfirmationState(callback);
  });
}

void AdsImpl::LoadConfirmationState(InitializeCallback callback) {
  ConfirmationStateManager::GetInstance()->Initialize([=](const bool success) {
    if (!success) {
      callback(/* success */ false);
      return;
    }

    LoadNotificationAdState(callback);
  });
}

void AdsImpl::LoadNotificationAdState(InitializeCallback callback) {
  NotificationAdManager::GetInstance()->Initialize([=](const bool success) {
    if (!success) {
      callback(/* success */ false);
      return;
    }

    Initialized(callback);
  });
}

void AdsImpl::Initialized(InitializeCallback callback) {
  BLOG(1, "Successfully initialized ads");

  is_initialized_ = true;

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kInitializedAds);

  MaybeUpdateIdleTimeThreshold();

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

  PurgeExpiredAdEvents();

  account_->Process();

  subdivision_targeting_->MaybeFetch();

  conversions_->Process();

  catalog_->MaybeFetch();

  MaybeServeNotificationAdsAtRegularIntervals();
}

void AdsImpl::MaybeServeNotificationAd() {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  notification_ad_serving_->MaybeServeAd();
}

bool AdsImpl::ShouldServeNotificationAdsAtRegularIntervals() const {
  return ShouldRewardUser() &&
         (BrowserManager::GetInstance()->IsBrowserActive() ||
          AdsClientHelper::GetInstance()->CanShowBackgroundNotifications()) &&
         settings::GetAdsPerHour() > 0;
}

void AdsImpl::MaybeServeNotificationAdsAtRegularIntervals() {
  if (!IsInitialized() || !PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  if (ShouldServeNotificationAdsAtRegularIntervals()) {
    notification_ad_serving_->StartServingAdsAtRegularIntervals();
  } else {
    notification_ad_serving_->StopServingAdsAtRegularIntervals();
  }
}

void AdsImpl::OnWalletDidUpdate(const WalletInfo& wallet) {
  MaybeServeNotificationAdsAtRegularIntervals();
}

void AdsImpl::OnStatementOfAccountsDidChange() {
  AdsClientHelper::GetInstance()->OnAdRewardsChanged();
}

void AdsImpl::OnDidUpdateCatalog(const CatalogInfo& catalog) {
  epsilon_greedy_bandit_resource_->LoadFromCatalog(catalog);
}

void AdsImpl::OnDidServeNotificationAd(const NotificationAdInfo& ad) {
  notification_ad_->FireEvent(ad.placement_id,
                              mojom::NotificationAdEventType::kServed);
}

void AdsImpl::OnNotificationAdViewed(const NotificationAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);

  CovariateManager::GetInstance()->SetNotificationAdServedAt(base::Time::Now());
}

void AdsImpl::OnNotificationAdClicked(const NotificationAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::NotificationAdEventType::kClicked});

  CovariateManager::GetInstance()->SetNotificationAdEvent(
      mojom::NotificationAdEventType::kClicked);
  CovariateManager::GetInstance()->LogTrainingInstance();
}

void AdsImpl::OnNotificationAdDismissed(const NotificationAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kDismissed);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::NotificationAdEventType::kDismissed});

  CovariateManager::GetInstance()->SetNotificationAdEvent(
      mojom::NotificationAdEventType::kDismissed);
  CovariateManager::GetInstance()->LogTrainingInstance();
}

void AdsImpl::OnNotificationAdTimedOut(const NotificationAdInfo& ad) {
  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::NotificationAdEventType::kTimedOut});

  CovariateManager::GetInstance()->SetNotificationAdEvent(
      mojom::NotificationAdEventType::kTimedOut);
  CovariateManager::GetInstance()->LogTrainingInstance();
}

void AdsImpl::OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) {
  new_tab_page_ad_->FireEvent(ad.placement_id, ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kServed);
}

void AdsImpl::OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void AdsImpl::OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void AdsImpl::OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnDidServeInlineContentAd(const InlineContentAdInfo& ad) {
  inline_content_ad_->FireEvent(ad.placement_id, ad.creative_instance_id,
                                mojom::InlineContentAdEventType::kServed);
}

void AdsImpl::OnInlineContentAdViewed(const InlineContentAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void AdsImpl::OnInlineContentAdClicked(const InlineContentAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnSearchResultAdViewed(const SearchResultAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void AdsImpl::OnSearchResultAdClicked(const SearchResultAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnDidTransferAd(const AdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kTransferred);
}

void AdsImpl::OnConversion(
    const ConversionQueueItemInfo& conversion_queue_item) {
  account_->Deposit(conversion_queue_item.creative_instance_id,
                    conversion_queue_item.ad_type,
                    ConfirmationType::kConversion);
}

}  // namespace ads
