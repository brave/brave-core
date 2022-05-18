/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_impl.h"

#include <utility>

#include "base/check.h"
#include "base/hash/hash.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_info.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ad_server/ad_server.h"
#include "bat/ads/internal/ad_server/catalog/catalog.h"
#include "bat/ads/internal/ad_server/catalog/catalog_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/platform_helper.h"
#include "bat/ads/internal/base/search_engine_util.h"
#include "bat/ads/internal/base/string_util.h"
#include "bat/ads/internal/base/time_formatting_util.h"
#include "bat/ads/internal/base/url_util.h"
#include "bat/ads/internal/browser_manager/browser_manager.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/conversions/conversions.h"
#include "bat/ads/internal/covariates/covariate_logs.h"
#include "bat/ads/internal/creatives/ad_notifications/ad_notification.h"
#include "bat/ads/internal/creatives/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ad.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ad.h"
#include "bat/ads/internal/creatives/promoted_content_ads/promoted_content_ad.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/database/database_initialize.h"
#include "bat/ads/internal/deprecated/client/client.h"
#include "bat/ads/internal/deprecated/confirmations/confirmations_state.h"
#include "bat/ads/internal/diagnostics/diagnostics.h"
#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_util.h"
#include "bat/ads/internal/features/features_util.h"
#include "bat/ads/internal/history/history.h"
#include "bat/ads/internal/legacy_migration/conversions/legacy_conversions_migration.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration.h"
#include "bat/ads/internal/privacy/tokens/token_generator.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_info.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversions_info.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversions_resource.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/resources/country_components.h"
#include "bat/ads/internal/resources/language_components.h"
#include "bat/ads/internal/serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/serving/inline_content_ads/inline_content_ad_serving.h"
#include "bat/ads/internal/serving/new_tab_page_ads/new_tab_page_ad_serving.h"
#include "bat/ads/internal/serving/targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/internal/studies/studies_util.h"
#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/tab_manager/tab_manager.h"
#include "bat/ads/internal/targeting/processors/behavioral/bandits/bandit_feedback_info.h"
#include "bat/ads/internal/targeting/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/targeting/processors/behavioral/purchase_intent/purchase_intent_processor.h"
#include "bat/ads/internal/targeting/processors/contextual/text_classification/text_classification_processor.h"
#include "bat/ads/internal/transfer/transfer.h"
#include "bat/ads/internal/user_activity/browsing/user_activity.h"
#include "bat/ads/internal/user_activity/idle_detection/idle_time.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/promoted_content_ad_info.h"
#include "bat/ads/statement_info.h"
#include "build/build_config.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"

namespace ads {

AdsImpl::AdsImpl(AdsClient* ads_client)
    : ads_client_helper_(std::make_unique<AdsClientHelper>(ads_client)),
      token_generator_(std::make_unique<privacy::TokenGenerator>()) {
  set(token_generator_.get());
}

AdsImpl::~AdsImpl() {
  account_->RemoveObserver(this);
  ad_notification_->RemoveObserver(this);
  ad_notification_serving_->RemoveObserver(this);
  ad_server_->RemoveObserver(this);
  transfer_->RemoveObserver(this);
  conversions_->RemoveObserver(this);
  inline_content_ad_->RemoveObserver(this);
  inline_content_ad_serving_->RemoveObserver(this);
  new_tab_page_ad_->RemoveObserver(this);
  new_tab_page_ad_serving_->RemoveObserver(this);
  promoted_content_ad_->RemoveObserver(this);
  search_result_ad_->RemoveObserver(this);
}

void AdsImpl::SetForTesting(privacy::TokenGeneratorInterface* token_generator) {
  DCHECK(token_generator);

  token_generator_.release();
  set(token_generator);
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

  InitializeBrowserManager();

  InitializeDatabase(callback);
}

void AdsImpl::Shutdown(ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(0, "Shutdown failed as not initialized");

    callback(/* success */ false);
    return;
  }

  ad_notifications_->CloseAndRemoveAll();

  callback(/* success */ true);
}

void AdsImpl::ChangeLocale(const std::string& locale) {
  subdivision_targeting_->MaybeFetchForLocale(locale);
  text_classification_resource_->Load();
  purchase_intent_resource_->Load();
  anti_targeting_resource_->Load();
  conversions_resource_->Load();
}

void AdsImpl::OnPrefChanged(const std::string& path) {
  if (path == prefs::kEnabled) {
    MaybeServeAdNotificationsAtRegularIntervals();
  }

  account_->OnPrefChanged(path);
  ad_notification_serving_->OnPrefChanged(path);
  subdivision_targeting_->OnPrefChanged(path);
}

void AdsImpl::OnHtmlLoaded(const int32_t tab_id,
                           const std::vector<GURL>& redirect_chain,
                           const std::string& html) {
  DCHECK(!redirect_chain.empty());

  if (!IsInitialized()) {
    return;
  }

  const uint32_t hash = base::FastHash(html);
  if (hash == last_html_loaded_hash_) {
    BLOG(1, "HTML content has not changed");
    return;
  }
  last_html_loaded_hash_ = hash;

  transfer_->MaybeTransferAd(tab_id, redirect_chain);
  conversions_->MaybeConvert(
      redirect_chain, html,
      conversions_resource_->get()->conversion_id_patterns);
}

void AdsImpl::OnTextLoaded(const int32_t tab_id,
                           const std::vector<GURL>& redirect_chain,
                           const std::string& text) {
  DCHECK(!redirect_chain.empty());

  if (!IsInitialized()) {
    return;
  }

  const uint32_t hash = base::FastHash(text);
  if (hash == last_text_loaded_hash_) {
    BLOG(1, "Text content has not changed");
    return;
  }
  last_text_loaded_hash_ = hash;

  const GURL& url = redirect_chain.back();

  if (!url.SchemeIsHTTPOrHTTPS()) {
    BLOG(1, "Visited URL is not supported");
    return;
  }

  const absl::optional<TabInfo> last_visible_tab =
      TabManager::Get()->GetLastVisible();
  if (!SameDomainOrHost(url,
                        last_visible_tab ? last_visible_tab->url : GURL())) {
    purchase_intent_processor_->Process(url);
  }

  if (IsSearchEngine(url)) {
    BLOG(1, "Search engine pages are not supported for text classification");
  } else {
    const std::string stripped_text = StripNonAlphaCharacters(text);
    text_classification_processor_->Process(stripped_text);
  }
}

void AdsImpl::OnUserGesture(const int32_t page_transition_type) {
  if (!IsInitialized()) {
    return;
  }

  user_activity_->RecordEventForPageTransition(page_transition_type);
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

  MaybeUpdateCatalog();

  if (!ShouldRewardUser()) {
    return;
  }

  if (WasLocked(was_locked)) {
    BLOG(1, "Ad notification not served: Screen was locked");
    return;
  }

  if (HasExceededMaximumIdleTime(idle_time)) {
    BLOG(1, "Ad notification not served: Exceeded maximum idle time");
    return;
  }

  MaybeServeAdNotification();
}

void AdsImpl::OnBrowserDidEnterForeground() {
  BrowserManager::Get()->OnDidEnterForeground();

  MaybeUpdateCatalog();

  MaybeServeAdNotificationsAtRegularIntervals();
}

void AdsImpl::OnBrowserDidEnterBackground() {
  BrowserManager::Get()->OnDidEnterBackground();

  MaybeServeAdNotificationsAtRegularIntervals();
}

void AdsImpl::OnMediaPlaying(const int32_t tab_id) {
  if (!IsInitialized()) {
    return;
  }

  TabManager::Get()->OnMediaPlaying(tab_id);
}

void AdsImpl::OnMediaStopped(const int32_t tab_id) {
  if (!IsInitialized()) {
    return;
  }

  TabManager::Get()->OnMediaStopped(tab_id);
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
    BrowserManager::Get()->OnDidBecomeActive();
  } else {
    BrowserManager::Get()->OnDidResignActive();
  }

  const bool is_visible = is_active && is_browser_active;
  TabManager::Get()->OnUpdated(tab_id, url, is_visible, is_incognito);
}

void AdsImpl::OnTabClosed(const int32_t tab_id) {
  if (!IsInitialized()) {
    return;
  }

  TabManager::Get()->OnClosed(tab_id);
}

void AdsImpl::OnWalletUpdated(const std::string& id, const std::string& seed) {
  account_->SetWallet(id, seed);
}

void AdsImpl::OnResourceComponentUpdated(const std::string& id) {
  if (kComponentLanguageIds.find(id) != kComponentLanguageIds.end()) {
    text_classification_resource_->Load();
  } else if (kComponentCountryIds.find(id) != kComponentCountryIds.end()) {
    purchase_intent_resource_->Load();
    anti_targeting_resource_->Load();
    conversions_resource_->Load();
  } else {
    BLOG(0, "Unknown resource for " << id);
  }
}

bool AdsImpl::GetAdNotification(const std::string& placement_id,
                                AdNotificationInfo* notification) {
  DCHECK(notification);
  return ad_notifications_->Get(placement_id, notification);
}

void AdsImpl::TriggerAdNotificationEvent(
    const std::string& placement_id,
    const mojom::AdNotificationEventType event_type) {
  ad_notification_->FireEvent(placement_id, event_type);
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

void AdsImpl::PurgeOrphanedAdEventsForType(const mojom::AdType ad_type) {
  PurgeOrphanedAdEvents(ad_type, [ad_type](const bool success) {
    if (!success) {
      BLOG(0, "Failed to purge orphaned ad events for " << ad_type);
      return;
    }

    BLOG(1, "Successfully purged orphaned ad events for " << ad_type);
  });
}

void AdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  Client::Get()->RemoveAllHistory();

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
  if (!IsInitialized() || !ShouldRewardUser()) {
    callback(/* success */ false, {});
    return;
  }

  account_->GetStatement(
      [callback](const bool success, const StatementInfo& statement) {
        callback(success, statement);
      });
}

void AdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  Diagnostics::Get()->Get(std::move(callback));
}

AdContentLikeActionType AdsImpl::ToggleAdThumbUp(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);

  const AdContentLikeActionType like_action_type =
      Client::Get()->ToggleAdThumbUp(ad_content);
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
      Client::Get()->ToggleAdThumbDown(ad_content);
  if (like_action_type == AdContentLikeActionType::kThumbsDown) {
    account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                      ConfirmationType::kDownvoted);
  }

  return like_action_type;
}

CategoryContentOptActionType AdsImpl::ToggleAdOptIn(
    const std::string& category,
    const CategoryContentOptActionType& action) {
  return Client::Get()->ToggleAdOptIn(category, action);
}

CategoryContentOptActionType AdsImpl::ToggleAdOptOut(
    const std::string& category,
    const CategoryContentOptActionType& action) {
  return Client::Get()->ToggleAdOptOut(category, action);
}

bool AdsImpl::ToggleSavedAd(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);

  const bool is_saved = Client::Get()->ToggleSavedAd(ad_content);
  if (is_saved) {
    account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                      ConfirmationType::kSaved);
  }

  return is_saved;
}

bool AdsImpl::ToggleFlaggedAd(const std::string& json) {
  AdContentInfo ad_content;
  ad_content.FromJson(json);

  const bool is_flagged = Client::Get()->ToggleFlaggedAd(ad_content);
  if (is_flagged) {
    account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                      ConfirmationType::kFlagged);
  }

  return is_flagged;
}

///////////////////////////////////////////////////////////////////////////////

void AdsImpl::set(privacy::TokenGeneratorInterface* token_generator) {
  DCHECK(token_generator);

  diagnostics_ = std::make_unique<Diagnostics>();

  browser_manager_ = std::make_unique<BrowserManager>();

  tab_manager_ = std::make_unique<TabManager>();

  account_ = std::make_unique<Account>(token_generator_.get());
  account_->AddObserver(this);

  epsilon_greedy_bandit_resource_ =
      std::make_unique<resource::EpsilonGreedyBandit>();
  epsilon_greedy_bandit_processor_ =
      std::make_unique<targeting::processor::EpsilonGreedyBandit>();

  text_classification_resource_ =
      std::make_unique<resource::TextClassification>();
  text_classification_processor_ =
      std::make_unique<targeting::processor::TextClassification>(
          text_classification_resource_.get());

  purchase_intent_resource_ = std::make_unique<resource::PurchaseIntent>();
  purchase_intent_processor_ =
      std::make_unique<targeting::processor::PurchaseIntent>(
          purchase_intent_resource_.get());

  anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();

  conversions_resource_ = std::make_unique<resource::Conversions>();

  subdivision_targeting_ =
      std::make_unique<targeting::geographic::SubdivisionTargeting>();

  ad_notification_serving_ = std::make_unique<ad_notifications::Serving>(
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  ad_notification_serving_->AddObserver(this);
  ad_notification_ = std::make_unique<AdNotification>();
  ad_notification_->AddObserver(this);
  ad_notifications_ = std::make_unique<AdNotifications>();

  ad_server_ = std::make_unique<AdServer>();
  ad_server_->AddObserver(this);

  transfer_ = std::make_unique<Transfer>();
  transfer_->AddObserver(this);

  inline_content_ad_serving_ = std::make_unique<inline_content_ads::Serving>(
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  inline_content_ad_serving_->AddObserver(this);
  inline_content_ad_ = std::make_unique<InlineContentAd>();
  inline_content_ad_->AddObserver(this);

  promoted_content_ad_ = std::make_unique<PromotedContentAd>();
  promoted_content_ad_->AddObserver(this);

  search_result_ad_ = std::make_unique<SearchResultAd>();
  search_result_ad_->AddObserver(this);

  confirmations_state_ = std::make_unique<ConfirmationsState>();

  client_ = std::make_unique<Client>();

  conversions_ = std::make_unique<Conversions>();
  conversions_->AddObserver(this);

  database_ = std::make_unique<database::Initialize>();

  new_tab_page_ad_serving_ = std::make_unique<new_tab_page_ads::Serving>(
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  new_tab_page_ad_serving_->AddObserver(this);
  new_tab_page_ad_ = std::make_unique<NewTabPageAd>();
  new_tab_page_ad_->AddObserver(this);

  user_activity_ = std::make_unique<UserActivity>();

  covariate_logs_ = std::make_unique<CovariateLogs>();
}

void AdsImpl::InitializeBrowserManager() {
  const bool is_browser_active = AdsClientHelper::Get()->IsBrowserActive();

  BrowserManager::Get()->SetForeground(is_browser_active);
  BrowserManager::Get()->SetActive(is_browser_active);
}

void AdsImpl::InitializeDatabase(InitializeCallback callback) {
  database_->CreateOrOpen([=](const bool success) {
    if (!success) {
      BLOG(0,
           "Failed to initialize database: " << database_->get_last_message());
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
  Client::Get()->Initialize([=](const bool success) {
    if (!success) {
      callback(/* success */ false);
      return;
    }

    LoadConfirmationsState(callback);
  });
}

void AdsImpl::LoadConfirmationsState(InitializeCallback callback) {
  ConfirmationsState::Get()->Initialize([=](const bool success) {
    if (!success) {
      callback(/* success */ false);
      return;
    }

    LoadAdNotificationsState(callback);
  });
}

void AdsImpl::LoadAdNotificationsState(InitializeCallback callback) {
  ad_notifications_->Initialize([=](const bool success) {
    if (!success) {
      callback(/* success */ false);
      return;
    }

    Initialized(callback);
  });
}

void AdsImpl::Initialized(InitializeCallback callback) {
  BLOG(1, "Successfully initialized ads");

  user_activity_->RecordEvent(UserActivityEventType::kInitializedAds);

  is_initialized_ = true;

  MaybeUpdateIdleTimeThreshold();

  callback(/* success */ true);

  Start();
}

void AdsImpl::Start() {
  LogFeatures();

  LogActiveStudies();

#if BUILDFLAG(IS_ANDROID)
  // Ad notifications do not sustain a reboot or update, so we should remove
  // orphaned ad notifications
  ad_notifications_->RemoveAllAfterReboot();
  ad_notifications_->RemoveAllAfterUpdate();
#endif

  CleanupAdEvents();

  OnStatementOfAccountsDidChange();

  account_->MaybeGetIssuers();
  account_->ProcessClearingCycle();

  subdivision_targeting_->MaybeFetchForCurrentLocale();

  conversions_->StartTimerIfReady();

  ad_server_->MaybeFetch();

  MaybeServeAdNotificationsAtRegularIntervals();
}

void AdsImpl::CleanupAdEvents() {
  PurgeExpiredAdEvents([](const bool success) {
    if (!success) {
      BLOG(1, "Failed to purge expired ad events");
      return;
    }

    BLOG(6, "Successfully purged expired ad events");
  });
}

void AdsImpl::MaybeUpdateCatalog() {
  if (!HasCatalogExpired()) {
    return;
  }

  ad_server_->MaybeFetch();
}

void AdsImpl::MaybeServeAdNotification() {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  ad_notification_serving_->MaybeServeAd();
}

bool AdsImpl::ShouldServeAdNotificationsAtRegularIntervals() const {
  return ShouldRewardUser() &&
         (BrowserManager::Get()->IsActive() ||
          AdsClientHelper::Get()->CanShowBackgroundNotifications()) &&
         settings::GetAdsPerHour() > 0;
}

void AdsImpl::MaybeServeAdNotificationsAtRegularIntervals() {
  if (!IsInitialized()) {
    return;
  }

  if (!PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  if (ShouldServeAdNotificationsAtRegularIntervals()) {
    ad_notification_serving_->StartServingAdsAtRegularIntervals();
  } else {
    ad_notification_serving_->StopServingAdsAtRegularIntervals();
  }
}

void AdsImpl::OnWalletDidUpdate(const WalletInfo& wallet) {
  BLOG(1, "Successfully set wallet");

  MaybeServeAdNotificationsAtRegularIntervals();
}

void AdsImpl::OnWalletDidChange(const WalletInfo& wallet) {
  BLOG(0, "Wallet changed");
}

void AdsImpl::OnInvalidWallet() {
  BLOG(0, "Failed to set wallet");
}

void AdsImpl::OnDidProcessDeposit(const TransactionInfo& transaction) {
  DCHECK(transaction.IsValid());

  BLOG(3, "Successfully processed deposit for "
              << transaction.ad_type << " with creative instance id "
              << transaction.creative_instance_id << " and "
              << transaction.confirmation_type << " valued at "
              << transaction.value);
}

void AdsImpl::OnFailedToProcessDeposit(
    const std::string& creative_instance_id,
    const AdType& ad_type,
    const ConfirmationType& confirmation_type) {
  BLOG(0, "Failed to process deposit for "
              << ad_type << " with creative instance id "
              << creative_instance_id << " and " << confirmation_type);
}

void AdsImpl::OnStatementOfAccountsDidChange() {
  AdsClientHelper::Get()->OnAdRewardsChanged();
}

void AdsImpl::OnCatalogUpdated(const Catalog& catalog) {
  epsilon_greedy_bandit_resource_->LoadFromCatalog(catalog);
}

void AdsImpl::OnDidServeAdNotification(const AdNotificationInfo& ad) {
  ad_notification_->FireEvent(ad.placement_id,
                              mojom::AdNotificationEventType::kServed);
}

void AdsImpl::OnAdNotificationViewed(const AdNotificationInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);

  covariate_logs_->SetAdNotificationServedAt(base::Time::Now());
}

void AdsImpl::OnAdNotificationClicked(const AdNotificationInfo& ad) {
  transfer_->set_last_clicked_ad(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::AdNotificationEventType::kClicked});

  covariate_logs_->SetAdNotificationClicked(true);
  covariate_logs_->LogTrainingInstance();
}

void AdsImpl::OnAdNotificationDismissed(const AdNotificationInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kDismissed);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::AdNotificationEventType::kDismissed});

  covariate_logs_->SetAdNotificationClicked(false);
  covariate_logs_->LogTrainingInstance();
}

void AdsImpl::OnAdNotificationTimedOut(const AdNotificationInfo& ad) {
  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::AdNotificationEventType::kTimedOut});

  covariate_logs_->SetAdNotificationClicked(false);
  covariate_logs_->LogTrainingInstance();
}

void AdsImpl::OnAdNotificationEventFailed(
    const std::string& placement_id,
    const mojom::AdNotificationEventType event_type) {
  BLOG(1, "Failed to fire ad notification "
              << event_type << " event for placement id " << placement_id);
}

void AdsImpl::OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) {
  new_tab_page_ad_->FireEvent(ad.placement_id, ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kServed);
}

void AdsImpl::OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) {
  if (!ShouldRewardUser()) {
    return;
  }

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void AdsImpl::OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) {
  transfer_->set_last_clicked_ad(ad);

  if (!ShouldRewardUser()) {
    return;
  }

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnNewTabPageAdEventFailed(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) {
  BLOG(1, "Failed to fire new tab page ad "
              << event_type << " event for placement id " << placement_id
              << " and creative instance id " << creative_instance_id);
}

void AdsImpl::OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void AdsImpl::OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) {
  transfer_->set_last_clicked_ad(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnPromotedContentAdEventFailed(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  BLOG(1, "Failed to fire promoted content ad "
              << event_type << " event for placement id " << placement_id
              << " and creative instance id " << creative_instance_id);
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
  transfer_->set_last_clicked_ad(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnInlineContentAdEventFailed(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  BLOG(1, "Failed to fire inline content ad "
              << event_type << " event for placement id " << placement_id
              << " and creative instance id " << creative_instance_id);
}

void AdsImpl::OnSearchResultAdViewed(const SearchResultAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void AdsImpl::OnSearchResultAdClicked(const SearchResultAdInfo& ad) {
  transfer_->set_last_clicked_ad(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnSearchResultAdEventFailed(
    const SearchResultAdInfo& ad,
    const mojom::SearchResultAdEventType event_type) {
  BLOG(1, "Failed to fire search result ad "
              << event_type << " event for placement_id " << ad.placement_id
              << " and creative instance id " << ad.creative_instance_id);
}

void AdsImpl::OnWillTransferAd(const AdInfo& ad, const base::Time time) {
  BLOG(1,
       "Transfer ad for " << ad.target_url << " " << FriendlyDateAndTime(time));
}

void AdsImpl::OnDidTransferAd(const AdInfo& ad) {
  BLOG(1, "Transferred ad for " << ad.target_url);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kTransferred);
}

void AdsImpl::OnCancelledTransfer(const AdInfo& ad, const int32_t tab_id) {
  BLOG(1, "Cancelled ad transfer for creative instance id "
              << ad.creative_instance_id << " with tab id " << tab_id);
}

void AdsImpl::OnFailedToTransferAd(const AdInfo& ad) {
  BLOG(1, "Failed to transfer ad for " << ad.target_url);
}

void AdsImpl::OnConversion(
    const ConversionQueueItemInfo& conversion_queue_item) {
  account_->Deposit(conversion_queue_item.creative_instance_id,
                    conversion_queue_item.ad_type,
                    ConfirmationType::kConversion);
}

}  // namespace ads
