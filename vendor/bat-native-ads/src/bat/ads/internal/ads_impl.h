/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_IMPL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_IMPL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "bat/ads/ads.h"
#include "bat/ads/history_filter_types.h"
#include "bat/ads/history_sort_types.h"
#include "bat/ads/internal/account/account_observer.h"
#include "bat/ads/internal/ad_events/inline_content_ads/inline_content_ad_event_handler_observer.h"
#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_observer.h"
#include "bat/ads/internal/ad_events/notification_ads/notification_ad_event_handler_observer.h"
#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad_event_handler_observer.h"
#include "bat/ads/internal/ad_events/search_result_ads/search_result_ad_event_handler_observer.h"
#include "bat/ads/internal/conversions/conversions_observer.h"
#include "bat/ads/internal/database/database_manager_observer.h"
#include "bat/ads/internal/serving/inline_content_ad_serving_observer.h"
#include "bat/ads/internal/serving/new_tab_page_ad_serving_observer.h"
#include "bat/ads/internal/serving/notification_ad_serving_observer.h"
#include "bat/ads/internal/transfer/transfer_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

class GURL;

namespace base {
class Time;
}  // namespace base

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace inline_content_ads {
class EventHandler;
class Serving;
}  // namespace inline_content_ads

namespace new_tab_page_ads {
class EventHandler;
class Serving;
}  // namespace new_tab_page_ads

namespace notification_ads {
class EventHandler;
class Serving;
}  // namespace notification_ads

namespace privacy {
class TokenGenerator;
}  // namespace privacy

namespace processor {
class EpsilonGreedyBandit;
class PurchaseIntent;
class TextClassification;
}  // namespace processor

namespace promoted_content_ads {
class EventHandler;
}  // namespace promoted_content_ads

namespace resource {
class AntiTargeting;
class EpsilonGreedyBandit;
class PurchaseIntent;
class TextClassification;
}  // namespace resource

namespace search_result_ads {
class EventHandler;
}  // namespace search_result_ads

class Account;
class AdsClientHelper;
class BrowserManager;
class Catalog;
class ClientStateManager;
class ConfirmationStateManager;
class Conversions;
class CovariateManager;
class DatabaseManager;
class DiagnosticManager;
class NotificationAdManager;
class LocaleManager;
class PrefManager;
class ResourceManager;
class TabManager;
class Transfer;
class UserActivityManager;
struct AdInfo;
struct ConversionQueueItemInfo;
struct HistoryInfo;
struct InlineContentAdInfo;
struct NewTabPageAdInfo;
struct NotificationAdInfo;
struct PromotedContentAdInfo;
struct SearchResultAdInfo;
struct WalletInfo;

class AdsImpl final : public Ads,
                      public AccountObserver,
                      public ConversionsObserver,
                      public inline_content_ads::EventHandlerObserver,
                      public inline_content_ads::ServingObserver,
                      public new_tab_page_ads::EventHandlerObserver,
                      public new_tab_page_ads::ServingObserver,
                      public notification_ads::EventHandlerObserver,
                      public notification_ads::ServingObserver,
                      public promoted_content_ads::EventHandlerObserver,
                      public search_result_ads::EventHandlerObserver,
                      public TransferObserver {
 public:
  explicit AdsImpl(AdsClient* ads_client);
  ~AdsImpl() override;

  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  bool IsInitialized() const;

  // Ads:
  void Initialize(InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void ChangeLocale(const std::string& locale) override;

  void OnPrefChanged(const std::string& path) override;

  void OnHtmlLoaded(const int32_t tab_id,
                    const std::vector<GURL>& redirect_chain,
                    const std::string& html) override;
  void OnTextLoaded(const int32_t tab_id,
                    const std::vector<GURL>& redirect_chain,
                    const std::string& text) override;

  void OnUserGesture(const int32_t page_transition_type) override;

  void OnIdle() override;
  void OnUnIdle(const int idle_time, const bool was_locked) override;

  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  void OnMediaPlaying(const int32_t tab_id) override;
  void OnMediaStopped(const int32_t tab_id) override;

  void OnTabUpdated(const int32_t tab_id,
                    const GURL& url,
                    const bool is_active,
                    const bool is_browser_active,
                    const bool is_incognito) override;
  void OnTabClosed(const int32_t tab_id) override;

  void OnWalletUpdated(const std::string& id, const std::string& seed) override;

  void OnResourceComponentUpdated(const std::string& id) override;

  bool GetNotificationAd(const std::string& placement_id,
                         NotificationAdInfo* notification_ad) override;
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      const mojom::NotificationAdEventType event_type) override;

  void GetNewTabPageAd(GetNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::NewTabPageAdEventType event_type) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::PromotedContentAdEventType event_type) override;

  void GetInlineContentAd(const std::string& dimensions,
                          GetInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::InlineContentAdEventType event_type) override;

  void TriggerSearchResultAdEvent(
      mojom::SearchResultAdPtr ad_mojom,
      const mojom::SearchResultAdEventType event_type,
      TriggerSearchResultAdEventCallback callback) override;

  void PurgeOrphanedAdEventsForType(
      const mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  HistoryInfo GetHistory(const HistoryFilterType filter_type,
                         const HistorySortType sort_type,
                         const base::Time from_time,
                         const base::Time to_time) override;
  void RemoveAllHistory(RemoveAllHistoryCallback callback) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  AdContentLikeActionType ToggleAdThumbUp(const std::string& json) override;
  AdContentLikeActionType ToggleAdThumbDown(const std::string& json) override;

  CategoryContentOptActionType ToggleAdOptIn(
      const std::string& category,
      const CategoryContentOptActionType& action) override;
  CategoryContentOptActionType ToggleAdOptOut(
      const std::string& category,
      const CategoryContentOptActionType& action) override;

  bool ToggleSavedAd(const std::string& json) override;

  bool ToggleFlaggedAd(const std::string& json) override;

 private:
  void InitializeDatabase(InitializeCallback callback);
  void MigrateConversions(InitializeCallback callback);
  void MigrateRewards(InitializeCallback callback);
  void LoadClientState(InitializeCallback callback);
  void LoadConfirmationState(InitializeCallback callback);
  void LoadNotificationAdState(InitializeCallback callback);
  void Initialized(InitializeCallback callback);

  void Start();

  void MaybeServeNotificationAd();
  bool ShouldServeNotificationAdsAtRegularIntervals() const;
  void MaybeServeNotificationAdsAtRegularIntervals();

  // AccountObserver:
  void OnWalletDidUpdate(const WalletInfo& wallet) override;
  void OnStatementOfAccountsDidChange() override;

  // notification_ads::ServingObserver:
  void OnDidServeNotificationAd(const NotificationAdInfo& ad) override;

  // notification_ads::EventHandlerObserver:
  void OnNotificationAdViewed(const NotificationAdInfo& ad) override;
  void OnNotificationAdClicked(const NotificationAdInfo& ad) override;
  void OnNotificationAdDismissed(const NotificationAdInfo& ad) override;
  void OnNotificationAdTimedOut(const NotificationAdInfo& ad) override;

  // new_tab_page_ads::ServingObserver:
  void OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) override;

  // new_tab_page_ads::EventHandlerObserver:
  void OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) override;
  void OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) override;

  // promoted_content_ads::EventHandlerObserver:
  void OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) override;
  void OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) override;

  // inline_content_ads::ServingObserver:
  void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) override;

  // inline_content_ads::EventHandlerObserver:
  void OnInlineContentAdViewed(const InlineContentAdInfo& ad) override;
  void OnInlineContentAdClicked(const InlineContentAdInfo& ad) override;

  // search_result_ads::EventHandlerObserver:
  void OnSearchResultAdViewed(const SearchResultAdInfo& ad) override;
  void OnSearchResultAdClicked(const SearchResultAdInfo& ad) override;

  // TransferObserver:
  void OnDidTransferAd(const AdInfo& ad) override;

  // ConversionsObserver:
  void OnConversion(
      const ConversionQueueItemInfo& conversion_queue_item) override;

  bool is_initialized_ = false;

  std::unique_ptr<AdsClientHelper> ads_client_helper_;

  std::unique_ptr<BrowserManager> browser_manager_;
  std::unique_ptr<ClientStateManager> client_state_manager_;
  std::unique_ptr<ConfirmationStateManager> confirmation_state_manager_;
  std::unique_ptr<CovariateManager> covariate_manager_;
  std::unique_ptr<DatabaseManager> database_manager_;
  std::unique_ptr<DiagnosticManager> diagnostic_manager_;
  std::unique_ptr<LocaleManager> locale_manager_;
  std::unique_ptr<NotificationAdManager> notification_ad_manager_;
  std::unique_ptr<PrefManager> pref_manager_;
  std::unique_ptr<ResourceManager> resource_manager_;
  std::unique_ptr<TabManager> tab_manager_;
  std::unique_ptr<UserActivityManager> user_activity_manager_;

  std::unique_ptr<privacy::TokenGenerator> token_generator_;
  std::unique_ptr<Account> account_;

  std::unique_ptr<Catalog> catalog_;

  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<resource::EpsilonGreedyBandit>
      epsilon_greedy_bandit_resource_;
  std::unique_ptr<resource::PurchaseIntent> purchase_intent_resource_;
  std::unique_ptr<resource::TextClassification> text_classification_resource_;

  std::unique_ptr<processor::EpsilonGreedyBandit>
      epsilon_greedy_bandit_processor_;
  std::unique_ptr<processor::PurchaseIntent> purchase_intent_processor_;
  std::unique_ptr<processor::TextClassification> text_classification_processor_;

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;

  std::unique_ptr<inline_content_ads::EventHandler>
      inline_content_ad_event_handler_;
  std::unique_ptr<inline_content_ads::Serving> inline_content_ad_serving_;

  std::unique_ptr<new_tab_page_ads::EventHandler>
      new_tab_page_ad_event_handler_;
  std::unique_ptr<new_tab_page_ads::Serving> new_tab_page_ad_serving_;

  std::unique_ptr<notification_ads::EventHandler>
      notification_ad_event_handler_;
  std::unique_ptr<notification_ads::Serving> notification_ad_serving_;

  std::unique_ptr<promoted_content_ads::EventHandler>
      promoted_content_ad_event_handler_;

  std::unique_ptr<search_result_ads::EventHandler>
      search_result_ad_event_handler_;

  std::unique_ptr<Conversions> conversions_;

  std::unique_ptr<Transfer> transfer_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_IMPL_H_
