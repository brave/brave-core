/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_impl.h"

#include <utility>

#include "base/time/time.h"
#include "bat/ads/ad_history_info.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards_util.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ad_server/ad_server.h"
#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_serving/inline_content_ads/inline_content_ad_serving.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/ad_targeting/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/ad_targeting/processors/behavioral/purchase_intent/purchase_intent_processor.h"
#include "bat/ads/internal/ad_targeting/processors/contextual/text_classification/text_classification_processor.h"
#include "bat/ads/internal/ad_transfer/ad_transfer.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notification.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad.h"
#include "bat/ads/internal/ads/promoted_content_ads/promoted_content_ad.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/browser_manager/browser_manager.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/conversions/conversions.h"
#include "bat/ads/internal/database/database_initialize.h"
#include "bat/ads/internal/features/features.h"
#include "bat/ads/internal/idle_time.h"
#include "bat/ads/internal/legacy_migration/legacy_conversion_migration.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/privacy/tokens/token_generator.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/resources/conversions/conversions_resource.h"
#include "bat/ads/internal/resources/country_components.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/resources/language_components.h"
#include "bat/ads/internal/search_engine/search_providers.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/internal/string_util.h"
#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/tab_manager/tab_manager.h"
#include "bat/ads/internal/url_util.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/promoted_content_ad_info.h"
#include "bat/ads/statement_info.h"

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
  ad_transfer_->RemoveObserver(this);
  conversions_->RemoveObserver(this);
  inline_content_ad_->RemoveObserver(this);
  inline_content_ad_serving_->RemoveObserver(this);
  new_tab_page_ad_->RemoveObserver(this);
  promoted_content_ad_->RemoveObserver(this);
}

void AdsImpl::set_for_testing(
    privacy::TokenGeneratorInterface* token_generator) {
  DCHECK(token_generator);

  token_generator_.release();
  set(token_generator);
}

bool AdsImpl::IsInitialized() {
  if (!is_initialized_ ||
      !AdsClientHelper::Get()->GetBooleanPref(prefs::kEnabled)) {
    return false;
  }

  return true;
}

void AdsImpl::Initialize(InitializeCallback callback) {
  BLOG(1, "Initializing ads");

  if (IsInitialized()) {
    BLOG(1, "Already initialized ads");
    callback(FAILED);
    return;
  }

  InitializeBrowserManager();

  InitializeDatabase(callback);
}

void AdsImpl::Shutdown(ShutdownCallback callback) {
  if (!is_initialized_) {
    BLOG(0, "Shutdown failed as not initialized");

    callback(FAILED);
    return;
  }

  ad_notifications_->CloseAndRemoveAll();

  callback(SUCCESS);
}

void AdsImpl::ChangeLocale(const std::string& locale) {
  subdivision_targeting_->MaybeFetchForLocale(locale);
  text_classification_resource_->Load();
  purchase_intent_resource_->Load();
  anti_targeting_resource_->Load();
  conversions_resource_->Load();
}

void AdsImpl::OnPrefChanged(const std::string& path) {
  if (path == prefs::kAdsPerHour) {
    ad_notification_serving_->OnAdsPerHourChanged();
  } else if (path == prefs::kAdsSubdivisionTargetingCode) {
    subdivision_targeting_->MaybeFetchForCurrentLocale();
  }
}

void AdsImpl::OnHtmlLoaded(const int32_t tab_id,
                           const std::vector<std::string>& redirect_chain,
                           const std::string& html) {
  DCHECK(!redirect_chain.empty());

  if (!IsInitialized()) {
    return;
  }

  const std::string url = redirect_chain.back();

  if (!DoesUrlHaveSchemeHTTPOrHTTPS(url)) {
    BLOG(1, "Visited URL is not supported");
    return;
  }

  const std::string original_url = redirect_chain.front();
  ad_transfer_->MaybeTransferAd(tab_id, original_url);
  conversions_->MaybeConvert(redirect_chain, html,
                             conversions_resource_->get());
}

void AdsImpl::OnTextLoaded(const int32_t tab_id,
                           const std::vector<std::string>& redirect_chain,
                           const std::string& text) {
  DCHECK(!redirect_chain.empty());

  if (!IsInitialized()) {
    return;
  }

  const std::string url = redirect_chain.back();

  if (!DoesUrlHaveSchemeHTTPOrHTTPS(url)) {
    BLOG(1, "Visited URL is not supported");
    return;
  }

  const base::Optional<TabInfo> last_visible_tab =
      TabManager::Get()->GetLastVisible();
  if (!SameDomainOrHost(url, last_visible_tab ? last_visible_tab->url : "")) {
    purchase_intent_processor_->Process(GURL(url));
  }

  if (SearchProviders::IsSearchEngine(url)) {
    BLOG(1, "Search engine pages are not supported for text classification");
  } else {
    const std::string stripped_text = StripNonAlphaCharacters(text);
    text_classification_processor_->Process(stripped_text);
  }
}

void AdsImpl::OnUserGesture(const int32_t page_transition_type) {
  user_activity_->RecordEventForPageTransitionFromInt(page_transition_type);
}

void AdsImpl::OnIdle() {
  BLOG(1, "Browser state changed to idle");
}

void AdsImpl::OnUnIdle(const int idle_time, const bool was_locked) {
  if (!IsInitialized()) {
    return;
  }

  MaybeUpdateIdleTimeThreshold();

  BLOG(1, "Browser state changed to unidle after "
              << base::TimeDelta::FromSeconds(idle_time));

  MaybeUpdateCatalog();

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

void AdsImpl::OnForeground() {
  BrowserManager::Get()->OnForegrounded();

  MaybeUpdateCatalog();

  MaybeServeAdNotificationsAtRegularIntervals();
}

void AdsImpl::OnBackground() {
  BrowserManager::Get()->OnBackgrounded();

  MaybeServeAdNotificationsAtRegularIntervals();
}

void AdsImpl::OnMediaPlaying(const int32_t tab_id) {
  TabManager::Get()->OnMediaPlaying(tab_id);
}

void AdsImpl::OnMediaStopped(const int32_t tab_id) {
  TabManager::Get()->OnMediaStopped(tab_id);
}

void AdsImpl::OnTabUpdated(const int32_t tab_id,
                           const std::string& url,
                           const bool is_active,
                           const bool is_browser_active,
                           const bool is_incognito) {
  if (is_browser_active) {
    BrowserManager::Get()->OnActive();
  } else {
    BrowserManager::Get()->OnInactive();
  }

  const bool is_visible = is_active && is_browser_active;
  TabManager::Get()->OnUpdated(tab_id, url, is_visible, is_incognito);
}

void AdsImpl::OnTabClosed(const int32_t tab_id) {
  TabManager::Get()->OnClosed(tab_id);

  ad_transfer_->Cancel(tab_id);
}

void AdsImpl::OnWalletUpdated(const std::string& id, const std::string& seed) {
  if (!ShouldRewardUser()) {
    return;
  }

  if (!account_->SetWallet(id, seed)) {
    BLOG(0, "Failed to set wallet");
    return;
  }

  BLOG(1, "Successfully set wallet");

  account_->TopUpUnblindedTokens();

  MaybeServeAdNotificationsAtRegularIntervals();
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

bool AdsImpl::GetAdNotification(const std::string& uuid,
                                AdNotificationInfo* notification) {
  DCHECK(notification);
  return ad_notifications_->Get(uuid, notification);
}

void AdsImpl::OnAdNotificationEvent(const std::string& uuid,
                                    const AdNotificationEventType event_type) {
  ad_notification_->FireEvent(uuid, event_type);
}

void AdsImpl::OnNewTabPageAdEvent(const std::string& uuid,
                                  const std::string& creative_instance_id,
                                  const NewTabPageAdEventType event_type) {
  if (event_type == NewTabPageAdEventType::kViewed) {
    // TODO(tmancey): We need to fire an ad served event until new tab page ads
    // are served by the ads library
    new_tab_page_ad_->FireEvent(uuid, creative_instance_id,
                                NewTabPageAdEventType::kServed);
  }

  new_tab_page_ad_->FireEvent(uuid, creative_instance_id, event_type);
}

void AdsImpl::OnPromotedContentAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const PromotedContentAdEventType event_type) {
  if (event_type == PromotedContentAdEventType::kViewed) {
    // TODO(tmancey): We need to fire an ad served event until promoted content
    // ads are served by the ads library
    promoted_content_ad_->FireEvent(uuid, creative_instance_id,
                                    PromotedContentAdEventType::kServed);
  }

  promoted_content_ad_->FireEvent(uuid, creative_instance_id, event_type);
}

void AdsImpl::GetInlineContentAd(const std::string& dimensions,
                                 GetInlineContentAdCallback callback) {
  inline_content_ad_serving_->MaybeServeAd(
      dimensions, [=](const bool success, const std::string& dimensions,
                      const InlineContentAdInfo& ad) {
        callback(success, dimensions, ad);
      });
}

void AdsImpl::OnInlineContentAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const InlineContentAdEventType event_type) {
  inline_content_ad_->FireEvent(uuid, creative_instance_id, event_type);
}

void AdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  Client::Get()->RemoveAllHistory();

  callback(SUCCESS);
}

void AdsImpl::ReconcileAdRewards() {
  if (!IsInitialized()) {
    return;
  }

  account_->Reconcile();
}

AdsHistoryInfo AdsImpl::GetAdsHistory(
    const AdsHistoryInfo::FilterType filter_type,
    const AdsHistoryInfo::SortType sort_type,
    const uint64_t from_timestamp,
    const uint64_t to_timestamp) {
  return history::Get(filter_type, sort_type, from_timestamp, to_timestamp);
}

void AdsImpl::GetAccountStatement(GetAccountStatementCallback callback) {
  StatementInfo statement;

  if (!IsInitialized()) {
    callback(/* success */ false, statement);
    return;
  }

  const int64_t to_timestamp =
      static_cast<int64_t>(base::Time::Now().ToDoubleT());

  statement = account_->GetStatement(0, to_timestamp);

  callback(/* success */ true, statement);
}

AdContentInfo::LikeAction AdsImpl::ToggleAdThumbUp(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContentInfo::LikeAction& action) {
  auto like_action = Client::Get()->ToggleAdThumbUp(creative_instance_id,
                                                    creative_set_id, action);
  if (like_action == AdContentInfo::LikeAction::kThumbsUp) {
    account_->Deposit(creative_instance_id, ConfirmationType::kUpvoted);
  }

  return like_action;
}

AdContentInfo::LikeAction AdsImpl::ToggleAdThumbDown(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const AdContentInfo::LikeAction& action) {
  auto like_action = Client::Get()->ToggleAdThumbDown(creative_instance_id,
                                                      creative_set_id, action);
  if (like_action == AdContentInfo::LikeAction::kThumbsDown) {
    account_->Deposit(creative_instance_id, ConfirmationType::kDownvoted);
  }

  return like_action;
}

CategoryContentInfo::OptAction AdsImpl::ToggleAdOptInAction(
    const std::string& category,
    const CategoryContentInfo::OptAction& action) {
  return Client::Get()->ToggleAdOptInAction(category, action);
}

CategoryContentInfo::OptAction AdsImpl::ToggleAdOptOutAction(
    const std::string& category,
    const CategoryContentInfo::OptAction& action) {
  return Client::Get()->ToggleAdOptOutAction(category, action);
}

bool AdsImpl::ToggleSaveAd(const std::string& creative_instance_id,
                           const std::string& creative_set_id,
                           const bool saved) {
  return Client::Get()->ToggleSaveAd(creative_instance_id, creative_set_id,
                                     saved);
}

bool AdsImpl::ToggleFlagAd(const std::string& creative_instance_id,
                           const std::string& creative_set_id,
                           const bool flagged) {
  auto flag_ad = Client::Get()->ToggleFlagAd(creative_instance_id,
                                             creative_set_id, flagged);
  if (flag_ad) {
    account_->Deposit(creative_instance_id, ConfirmationType::kFlagged);
  }

  return flag_ad;
}

///////////////////////////////////////////////////////////////////////////////

void AdsImpl::set(privacy::TokenGeneratorInterface* token_generator) {
  DCHECK(token_generator);

  account_ = std::make_unique<Account>(token_generator_.get());
  account_->AddObserver(this);

  epsilon_greedy_bandit_resource_ =
      std::make_unique<resource::EpsilonGreedyBandit>();
  epsilon_greedy_bandit_processor_ =
      std::make_unique<ad_targeting::processor::EpsilonGreedyBandit>();

  text_classification_resource_ =
      std::make_unique<resource::TextClassification>();
  text_classification_processor_ =
      std::make_unique<ad_targeting::processor::TextClassification>(
          text_classification_resource_.get());

  purchase_intent_resource_ = std::make_unique<resource::PurchaseIntent>();
  purchase_intent_processor_ =
      std::make_unique<ad_targeting::processor::PurchaseIntent>(
          purchase_intent_resource_.get());

  anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();

  conversions_resource_ = std::make_unique<resource::Conversions>();

  ad_targeting_ = std::make_unique<AdTargeting>();
  subdivision_targeting_ =
      std::make_unique<ad_targeting::geographic::SubdivisionTargeting>();

  ad_notification_serving_ = std::make_unique<ad_notifications::AdServing>(
      ad_targeting_.get(), subdivision_targeting_.get(),
      anti_targeting_resource_.get());
  ad_notification_serving_->AddObserver(this);
  ad_notification_ = std::make_unique<AdNotification>();
  ad_notification_->AddObserver(this);
  ad_notifications_ = std::make_unique<AdNotifications>();

  ad_server_ = std::make_unique<AdServer>();
  ad_server_->AddObserver(this);

  ad_transfer_ = std::make_unique<AdTransfer>();
  ad_transfer_->AddObserver(this);

  inline_content_ad_serving_ = std::make_unique<inline_content_ads::AdServing>(
      ad_targeting_.get(), subdivision_targeting_.get(),
      anti_targeting_resource_.get());
  inline_content_ad_serving_->AddObserver(this);
  inline_content_ad_ = std::make_unique<InlineContentAd>();
  inline_content_ad_->AddObserver(this);

  promoted_content_ad_ = std::make_unique<PromotedContentAd>();
  promoted_content_ad_->AddObserver(this);

  client_ = std::make_unique<Client>();

  conversions_ = std::make_unique<Conversions>();
  conversions_->AddObserver(this);

  database_ = std::make_unique<database::Initialize>();

  new_tab_page_ad_ = std::make_unique<NewTabPageAd>();
  new_tab_page_ad_->AddObserver(this);

  browser_manager_ = std::make_unique<BrowserManager>();

  tab_manager_ = std::make_unique<TabManager>();

  user_activity_ = std::make_unique<UserActivity>();
}

void AdsImpl::InitializeBrowserManager() {
  const bool is_foreground = AdsClientHelper::Get()->IsForeground();

  BrowserManager::Get()->SetForegrounded(is_foreground);
  BrowserManager::Get()->SetActive(is_foreground ? true : false);
}

void AdsImpl::InitializeDatabase(InitializeCallback callback) {
  database_->CreateOrOpen([=](const Result result) {
    if (result != SUCCESS) {
      BLOG(0,
           "Failed to initialize database: " << database_->get_last_message());
      callback(FAILED);
      return;
    }

    RebuildAdEventsFromDatabase();

    MigrateConversions(callback);
  });
}

void AdsImpl::MigrateConversions(InitializeCallback callback) {
  conversions::Migrate([=](const Result result) {
    if (result != SUCCESS) {
      callback(FAILED);
      return;
    }

    LoadClientState(callback);
  });
}

void AdsImpl::LoadClientState(InitializeCallback callback) {
  Client::Get()->Initialize([=](const Result result) {
    if (result != SUCCESS) {
      callback(FAILED);
      return;
    }

    LoadConfirmationsState(callback);
  });
}

void AdsImpl::LoadConfirmationsState(InitializeCallback callback) {
  ConfirmationsState::Get()->Initialize([=](const Result result) {
    if (result != SUCCESS) {
      callback(FAILED);
      return;
    }

    LoadAdNotificationsState(callback);
  });
}

void AdsImpl::LoadAdNotificationsState(InitializeCallback callback) {
  ad_notifications_->Initialize([=](const Result result) {
    if (result != SUCCESS) {
      callback(FAILED);
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

  callback(SUCCESS);

  Start();
}

void AdsImpl::Start() {
#if defined(OS_ANDROID)
  // Ad notifications do not sustain a reboot or update, so we should remove
  // orphaned ad notifications
  ad_notifications_->RemoveAllAfterReboot();
  ad_notifications_->RemoveAllAfterUpdate();
#endif

  CleanupAdEvents();

  account_->Reconcile();
  account_->ProcessTransactions();

  subdivision_targeting_->MaybeFetchForCurrentLocale();

  conversions_->StartTimerIfReady();

  ad_server_->MaybeFetch();

  features::Log();

  MaybeServeAdNotificationsAtRegularIntervals();
}

void AdsImpl::CleanupAdEvents() {
  PurgeExpiredAdEvents([](const Result result) {
    if (result != Result::SUCCESS) {
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

void AdsImpl::MaybeServeAdNotificationsAtRegularIntervals() {
  if (!PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  if ((BrowserManager::Get()->IsActive() ||
       AdsClientHelper::Get()->CanShowBackgroundNotifications()) &&
      settings::GetAdsPerHour() > 0) {
    ad_notification_serving_->StartServingAdsAtRegularIntervals();
  } else {
    ad_notification_serving_->StopServingAdsAtRegularIntervals();
  }
}

void AdsImpl::OnAdRewardsChanged() {
  AdsClientHelper::Get()->OnAdRewardsChanged();
}

void AdsImpl::OnTransactionsChanged() {
  AdsClientHelper::Get()->OnAdRewardsChanged();
}

void AdsImpl::OnCatalogUpdated(const Catalog& catalog) {
  account_->SetCatalogIssuers(catalog.GetIssuers());
  account_->TopUpUnblindedTokens();

  epsilon_greedy_bandit_resource_->LoadFromCatalog(catalog);
}

void AdsImpl::OnDidServeAdNotification(const AdNotificationInfo& ad) {
  ad_notification_->FireEvent(ad.uuid, AdNotificationEventType::kServed);
}

void AdsImpl::OnAdNotificationViewed(const AdNotificationInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ConfirmationType::kViewed);
}

void AdsImpl::OnAdNotificationClicked(const AdNotificationInfo& ad) {
  ad_transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ConfirmationType::kClicked);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, AdNotificationEventType::kClicked});
}

void AdsImpl::OnAdNotificationDismissed(const AdNotificationInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ConfirmationType::kDismissed);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, AdNotificationEventType::kDismissed});
}

void AdsImpl::OnAdNotificationTimedOut(const AdNotificationInfo& ad) {
  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, AdNotificationEventType::kTimedOut});
}

void AdsImpl::OnAdNotificationEventFailed(
    const std::string& uuid,
    const AdNotificationEventType event_type) {
  BLOG(1, "Failed to fire ad notification " << event_type << " event for uuid "
                                            << uuid);
}

void AdsImpl::OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ConfirmationType::kViewed);
}

void AdsImpl::OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) {
  ad_transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ConfirmationType::kClicked);
}

void AdsImpl::OnNewTabPageAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const NewTabPageAdEventType event_type) {
  BLOG(1, "Failed to fire new tab page ad "
              << event_type << " event for uuid " << uuid
              << " and creative instance id " << creative_instance_id);
}

void AdsImpl::OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ConfirmationType::kViewed);
}

void AdsImpl::OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) {
  ad_transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ConfirmationType::kClicked);
}

void AdsImpl::OnPromotedContentAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const PromotedContentAdEventType event_type) {
  BLOG(1, "Failed to fire promoted content ad "
              << event_type << " event for uuid " << uuid
              << " and creative instance id " << creative_instance_id);
}

void AdsImpl::OnDidServeInlineContentAd(const InlineContentAdInfo& ad) {
  inline_content_ad_->FireEvent(ad.uuid, ad.creative_instance_id,
                                InlineContentAdEventType::kServed);
}

void AdsImpl::OnInlineContentAdViewed(const InlineContentAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ConfirmationType::kViewed);
}

void AdsImpl::OnInlineContentAdClicked(const InlineContentAdInfo& ad) {
  ad_transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ConfirmationType::kClicked);
}

void AdsImpl::OnInlineContentAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const InlineContentAdEventType event_type) {
  BLOG(1, "Failed to fire inline content ad "
              << event_type << " event for uuid " << uuid
              << " and creative instance id " << creative_instance_id);
}

void AdsImpl::OnAdTransfer(const AdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ConfirmationType::kTransferred);
}

void AdsImpl::OnConversion(
    const ConversionQueueItemInfo& conversion_queue_item) {
  account_->Deposit(conversion_queue_item.creative_instance_id,
                    ConfirmationType::kConversion);
}

}  // namespace ads
