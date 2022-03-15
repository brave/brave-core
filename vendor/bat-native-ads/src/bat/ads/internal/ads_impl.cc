/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_impl.h"

#include <utility>

#include "base/check.h"
#include "base/hash/hash.h"
#include "base/time/time.h"
#include "bat/ads/ad_history_info.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics.h"
#include "bat/ads/internal/ad_diagnostics/last_unidle_timestamp_ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ad_server/ad_server.h"
#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_serving/inline_content_ads/inline_content_ad_serving.h"
#include "bat/ads/internal/ad_serving/new_tab_page_ads/new_tab_page_ad_serving.h"
#include "bat/ads/internal/ad_targeting/processors/behavioral/bandits/bandit_feedback_info.h"
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
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/conversions/conversions.h"
#include "bat/ads/internal/database/database_initialize.h"
#include "bat/ads/internal/features/features.h"
#include "bat/ads/internal/federated/covariate_logs.h"
#include "bat/ads/internal/legacy_migration/conversions/legacy_conversion_migration.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/privacy/tokens/token_generator.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/resources/conversions/conversions_resource.h"
#include "bat/ads/internal/resources/country_components.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_info.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/resources/language_components.h"
#include "bat/ads/internal/search_engine/search_providers.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/internal/string_util.h"
#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/tab_manager/tab_manager.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/internal/url_util.h"
#include "bat/ads/internal/user_activity/idle_time.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/promoted_content_ad_info.h"
#include "bat/ads/statement_info.h"
#include "build/build_config.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
  new_tab_page_ad_serving_->RemoveObserver(this);
  promoted_content_ad_->RemoveObserver(this);
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
    account_->MaybeGetIssuers();

    MaybeServeAdNotificationsAtRegularIntervals();
  } else if (path == prefs::kAdsPerHour) {
    ad_notification_serving_->OnPrefChanged(path);
  } else if (path == prefs::kAutoDetectedAdsSubdivisionTargetingCode ||
             path == prefs::kAdsSubdivisionTargetingCode) {
    subdivision_targeting_->OnPrefChanged(path);
  }
}

void AdsImpl::OnHtmlLoaded(const int32_t tab_id,
                           const std::vector<std::string>& redirect_chain,
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

  ad_transfer_->MaybeTransferAd(tab_id, redirect_chain);
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

  const uint32_t hash = base::FastHash(text);
  if (hash == last_text_loaded_hash_) {
    BLOG(1, "Text content has not changed");
    return;
  }
  last_text_loaded_hash_ = hash;

  const std::string url = redirect_chain.back();

  if (!DoesUrlHaveSchemeHTTPOrHTTPS(url)) {
    BLOG(1, "Visited URL is not supported");
    return;
  }

  const absl::optional<TabInfo> last_visible_tab =
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
  if (!IsInitialized()) {
    return;
  }

  user_activity_->RecordEventForPageTransitionFromInt(page_transition_type);
}

void AdsImpl::OnIdle() {
  BLOG(1, "Browser state changed to idle");
}

void AdsImpl::OnUnIdle(const int idle_time, const bool was_locked) {
  if (!IsInitialized()) {
    return;
  }

  auto last_unidle_timestamp_diagnostics =
      std::make_unique<LastUnIdleTimestampAdDiagnosticsEntry>();
  last_unidle_timestamp_diagnostics->SetLastUnIdleTimestamp(base::Time::Now());
  AdDiagnostics::Get()->SetDiagnosticsEntry(
      std::move(last_unidle_timestamp_diagnostics));

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
                           const std::string& url,
                           const bool is_active,
                           const bool is_browser_active,
                           const bool is_incognito) {
  if (!IsInitialized()) {
    return;
  }

  if (is_browser_active) {
    BrowserManager::Get()->OnActive();
  } else {
    BrowserManager::Get()->OnInactive();
  }

  const bool is_visible = is_active && is_browser_active;
  TabManager::Get()->OnUpdated(tab_id, url, is_visible, is_incognito);
}

void AdsImpl::OnTabClosed(const int32_t tab_id) {
  if (!IsInitialized()) {
    return;
  }

  TabManager::Get()->OnClosed(tab_id);

  ad_transfer_->Cancel(tab_id);
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

bool AdsImpl::GetAdNotification(const std::string& uuid,
                                AdNotificationInfo* notification) {
  DCHECK(notification);
  return ad_notifications_->Get(uuid, notification);
}

void AdsImpl::OnAdNotificationEvent(
    const std::string& uuid,
    const mojom::AdNotificationEventType event_type) {
  ad_notification_->FireEvent(uuid, event_type);
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

void AdsImpl::OnNewTabPageAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) {
  new_tab_page_ad_->FireEvent(uuid, creative_instance_id, event_type);
}

void AdsImpl::OnPromotedContentAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  promoted_content_ad_->FireEvent(uuid, creative_instance_id, event_type);
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

void AdsImpl::OnInlineContentAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  inline_content_ad_->FireEvent(uuid, creative_instance_id, event_type);
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

AdsHistoryInfo AdsImpl::GetAdsHistory(const AdsHistoryFilterType filter_type,
                                      const AdsHistorySortType sort_type,
                                      const double from_timestamp,
                                      const double to_timestamp) {
  if (!IsInitialized()) {
    return {};
  }

  const base::Time from = base::Time::FromDoubleT(from_timestamp);
  const base::Time to = base::Time::FromDoubleT(to_timestamp);

  return history::Get(filter_type, sort_type, from, to);
}

void AdsImpl::GetAccountStatement(GetAccountStatementCallback callback) {
  if (!IsInitialized() || !ShouldRewardUser()) {
    callback(/* success */ false, {});
    return;
  }

  account_->GetStatement(
      [callback](const bool success, const StatementInfo& statement) {
        callback(success, statement);
      });
}

void AdsImpl::GetAdDiagnostics(GetAdDiagnosticsCallback callback) {
  AdDiagnostics::Get()->GetAdDiagnostics(std::move(callback));
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

  ad_diagnostics_ = std::make_unique<AdDiagnostics>();

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

  subdivision_targeting_ =
      std::make_unique<ad_targeting::geographic::SubdivisionTargeting>();

  ad_notification_serving_ = std::make_unique<ad_notifications::AdServing>(
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  ad_notification_serving_->AddObserver(this);
  ad_notification_ = std::make_unique<AdNotification>();
  ad_notification_->AddObserver(this);
  ad_notifications_ = std::make_unique<AdNotifications>();

  ad_server_ = std::make_unique<AdServer>();
  ad_server_->AddObserver(this);

  ad_transfer_ = std::make_unique<AdTransfer>();
  ad_transfer_->AddObserver(this);

  inline_content_ad_serving_ = std::make_unique<inline_content_ads::AdServing>(
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  inline_content_ad_serving_->AddObserver(this);
  inline_content_ad_ = std::make_unique<InlineContentAd>();
  inline_content_ad_->AddObserver(this);

  promoted_content_ad_ = std::make_unique<PromotedContentAd>();
  promoted_content_ad_->AddObserver(this);

  confirmations_state_ = std::make_unique<ConfirmationsState>();

  client_ = std::make_unique<Client>();

  conversions_ = std::make_unique<Conversions>();
  conversions_->AddObserver(this);

  database_ = std::make_unique<database::Initialize>();

  new_tab_page_ad_serving_ = std::make_unique<new_tab_page_ads::AdServing>(
      subdivision_targeting_.get(), anti_targeting_resource_.get());
  new_tab_page_ad_serving_->AddObserver(this);
  new_tab_page_ad_ = std::make_unique<NewTabPageAd>();
  new_tab_page_ad_->AddObserver(this);

  browser_manager_ = std::make_unique<BrowserManager>();

  tab_manager_ = std::make_unique<TabManager>();

  user_activity_ = std::make_unique<UserActivity>();

  covariate_logs_ = std::make_unique<CovariateLogs>();
}

void AdsImpl::InitializeBrowserManager() {
  const bool is_foreground = AdsClientHelper::Get()->IsForeground();

  BrowserManager::Get()->SetForegrounded(is_foreground);
  BrowserManager::Get()->SetActive(is_foreground ? true : false);
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

  features::Log();

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
              << std::string(transaction.ad_type)
              << " with creative instance id "
              << transaction.creative_instance_id << " and "
              << std::string(transaction.confirmation_type) << " valued at "
              << transaction.value);
}

void AdsImpl::OnFailedToProcessDeposit(
    const std::string& creative_instance_id,
    const AdType& ad_type,
    const ConfirmationType& confirmation_type) {
  BLOG(0, "Failed to process deposit for " << std::string(ad_type)
                                           << " with creative instance id "
                                           << creative_instance_id << " and "
                                           << std::string(confirmation_type));
}

void AdsImpl::OnStatementOfAccountsDidChange() {
  AdsClientHelper::Get()->OnAdRewardsChanged();
}

void AdsImpl::OnCatalogUpdated(const Catalog& catalog) {
  epsilon_greedy_bandit_resource_->LoadFromCatalog(catalog);
}

void AdsImpl::OnDidServeAdNotification(const AdNotificationInfo& ad) {
  ad_notification_->FireEvent(ad.uuid, mojom::AdNotificationEventType::kServed);
}

void AdsImpl::OnAdNotificationViewed(const AdNotificationInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);

  const base::Time impression_served_at = base::Time::Now();
  covariate_logs_->SetAdNotificationImpressionServedAt(impression_served_at);
}

void AdsImpl::OnAdNotificationClicked(const AdNotificationInfo& ad) {
  ad_transfer_->set_last_clicked_ad(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::AdNotificationEventType::kClicked});

  covariate_logs_->SetAdNotificationWasClicked(true);
  covariate_logs_->LogTrainingCovariates();
}

void AdsImpl::OnAdNotificationDismissed(const AdNotificationInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kDismissed);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::AdNotificationEventType::kDismissed});

  covariate_logs_->SetAdNotificationWasClicked(false);
  covariate_logs_->LogTrainingCovariates();
}

void AdsImpl::OnAdNotificationTimedOut(const AdNotificationInfo& ad) {
  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::AdNotificationEventType::kTimedOut});

  covariate_logs_->SetAdNotificationWasClicked(false);
  covariate_logs_->LogTrainingCovariates();
}

void AdsImpl::OnAdNotificationEventFailed(
    const std::string& uuid,
    const mojom::AdNotificationEventType event_type) {
  BLOG(1, "Failed to fire ad notification " << event_type << " event for uuid "
                                            << uuid);
}

void AdsImpl::OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) {
  new_tab_page_ad_->FireEvent(ad.uuid, ad.creative_instance_id,
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
  ad_transfer_->set_last_clicked_ad(ad);

  if (!ShouldRewardUser()) {
    return;
  }

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnNewTabPageAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) {
  BLOG(1, "Failed to fire new tab page ad "
              << event_type << " event for uuid " << uuid
              << " and creative instance id " << creative_instance_id);
}

void AdsImpl::OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void AdsImpl::OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) {
  ad_transfer_->set_last_clicked_ad(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnPromotedContentAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  BLOG(1, "Failed to fire promoted content ad "
              << event_type << " event for uuid " << uuid
              << " and creative instance id " << creative_instance_id);
}

void AdsImpl::OnDidServeInlineContentAd(const InlineContentAdInfo& ad) {
  inline_content_ad_->FireEvent(ad.uuid, ad.creative_instance_id,
                                mojom::InlineContentAdEventType::kServed);
}

void AdsImpl::OnInlineContentAdViewed(const InlineContentAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void AdsImpl::OnInlineContentAdClicked(const InlineContentAdInfo& ad) {
  ad_transfer_->set_last_clicked_ad(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

void AdsImpl::OnInlineContentAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  BLOG(1, "Failed to fire inline content ad "
              << event_type << " event for uuid " << uuid
              << " and creative instance id " << creative_instance_id);
}

void AdsImpl::OnWillTransferAd(const AdInfo& ad, const base::Time& time) {
  BLOG(1,
       "Transfer ad for " << ad.target_url << " " << FriendlyDateAndTime(time));
}

void AdsImpl::OnDidTransferAd(const AdInfo& ad) {
  BLOG(1, "Transferred ad for " << ad.target_url);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kTransferred);
}

void AdsImpl::OnCancelledAdTransfer(const AdInfo& ad, const int32_t tab_id) {
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
