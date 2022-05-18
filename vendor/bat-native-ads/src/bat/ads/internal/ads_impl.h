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
#include "bat/ads/internal/ad_server/ad_server_observer.h"
#include "bat/ads/internal/conversions/conversions_observer.h"
#include "bat/ads/internal/creatives/ad_notifications/ad_notification_observer.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ad_observer.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ad_observer.h"
#include "bat/ads/internal/creatives/promoted_content_ads/promoted_content_ad_observer.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_observer.h"
#include "bat/ads/internal/serving/ad_notifications/ad_notification_serving_observer.h"
#include "bat/ads/internal/serving/inline_content_ads/inline_content_ad_serving_observer.h"
#include "bat/ads/internal/serving/new_tab_page_ads/new_tab_page_ad_serving_observer.h"
#include "bat/ads/internal/transfer/transfer_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

class GURL;

namespace base {
class Time;
}  // namespace base

namespace ads {

namespace ad_notifications {
class Serving;
}  // namespace ad_notifications

namespace inline_content_ads {
class Serving;
}  // namespace inline_content_ads

namespace new_tab_page_ads {
class Serving;
}  // namespace new_tab_page_ads

namespace targeting {

namespace processor {
class EpsilonGreedyBandit;
class PurchaseIntent;
class TextClassification;
}  // namespace processor

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

}  // namespace targeting

namespace resource {
class AntiTargeting;
class Conversions;
class EpsilonGreedyBandit;
class PurchaseIntent;
class TextClassification;
}  // namespace resource

namespace database {
class Initialize;
}  // namespace database

namespace privacy {
class TokenGenerator;
class TokenGeneratorInterface;
}  // namespace privacy

class Account;
class Diagnostics;
class AdNotification;
class AdNotifications;
class AdServer;
class Transfer;
class AdsClientHelper;
class BrowserManager;
class Catalog;
class Client;
class Conversions;
class ConfirmationsState;
class CovariateLogs;
class InlineContentAd;
class NewTabPageAd;
class PromotedContentAd;
class SearchResultAd;
class TabManager;
class UserActivity;
struct AdInfo;
struct AdNotificationInfo;
struct HistoryInfo;
struct ConversionQueueItemInfo;
struct InlineContentAdInfo;
struct NewTabPageAdInfo;
struct PromotedContentAdInfo;
struct SearchResultAdInfo;
struct WalletInfo;

class AdsImpl final : public Ads,
                      public AccountObserver,
                      public AdNotificationObserver,
                      public AdNotificationServingObserver,
                      public AdServerObserver,
                      public TransferObserver,
                      public InlineContentAdObserver,
                      public InlineContentServingObserver,
                      public ConversionsObserver,
                      public NewTabPageAdObserver,
                      public NewTabPageServingObserver,
                      public PromotedContentAdObserver,
                      public SearchResultAdObserver {
 public:
  explicit AdsImpl(AdsClient* ads_client);
  ~AdsImpl() override;

  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  void SetForTesting(privacy::TokenGeneratorInterface* token_generator);

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

  bool GetAdNotification(const std::string& placement_id,
                         AdNotificationInfo* ad_notification) override;
  void TriggerAdNotificationEvent(
      const std::string& placement_id,
      const mojom::AdNotificationEventType event_type) override;

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

  void PurgeOrphanedAdEventsForType(const mojom::AdType ad_type) override;

  void RemoveAllHistory(RemoveAllHistoryCallback callback) override;

  HistoryInfo GetHistory(const HistoryFilterType filter_type,
                         const HistorySortType sort_type,
                         const base::Time from_time,
                         const base::Time to_time) override;

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
  void set(privacy::TokenGeneratorInterface* token_generator);

  void InitializeBrowserManager();
  void InitializeDatabase(InitializeCallback callback);
  void MigrateConversions(InitializeCallback callback);
  void MigrateRewards(InitializeCallback callback);
  void LoadClientState(InitializeCallback callback);
  void LoadConfirmationsState(InitializeCallback callback);
  void LoadAdNotificationsState(InitializeCallback callback);
  void Initialized(InitializeCallback callback);

  void Start();

  void CleanupAdEvents();

  void MaybeUpdateCatalog();

  void MaybeServeAdNotification();

  bool ShouldServeAdNotificationsAtRegularIntervals() const;
  void MaybeServeAdNotificationsAtRegularIntervals();

  // AccountObserver:
  void OnWalletDidUpdate(const WalletInfo& wallet) override;
  void OnWalletDidChange(const WalletInfo& wallet) override;
  void OnInvalidWallet() override;
  void OnDidProcessDeposit(const TransactionInfo& transaction) override;
  void OnFailedToProcessDeposit(
      const std::string& creative_instnce_id,
      const AdType& ad_type,
      const ConfirmationType& confirmation_type) override;
  void OnStatementOfAccountsDidChange() override;

  // AdServerObserver:
  void OnCatalogUpdated(const Catalog& catalog) override;

  // AdNotificationServingObserver:
  void OnDidServeAdNotification(const AdNotificationInfo& ad) override;

  // AdNotificationObserver:
  void OnAdNotificationViewed(const AdNotificationInfo& ad) override;
  void OnAdNotificationClicked(const AdNotificationInfo& ad) override;
  void OnAdNotificationDismissed(const AdNotificationInfo& ad) override;
  void OnAdNotificationTimedOut(const AdNotificationInfo& ad) override;
  void OnAdNotificationEventFailed(
      const std::string& placement_id,
      const mojom::AdNotificationEventType event_type) override;

  // NewTabPageServingObserver:
  void OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) override;

  // NewTabPageAdObserver:
  void OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) override;
  void OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) override;
  void OnNewTabPageAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::NewTabPageAdEventType event_type) override;

  // PromotedContentAdObserver:
  void OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) override;
  void OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) override;
  void OnPromotedContentAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::PromotedContentAdEventType event_type) override;

  // InlineContentServingObserver:
  void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) override;

  // InlineContentAdObserver:
  void OnInlineContentAdViewed(const InlineContentAdInfo& ad) override;
  void OnInlineContentAdClicked(const InlineContentAdInfo& ad) override;
  void OnInlineContentAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::InlineContentAdEventType event_type) override;

  // SearchResultAdObserver:
  void OnSearchResultAdViewed(const SearchResultAdInfo& ad) override;
  void OnSearchResultAdClicked(const SearchResultAdInfo& ad) override;
  void OnSearchResultAdEventFailed(
      const SearchResultAdInfo& ad,
      const mojom::SearchResultAdEventType event_type) override;

  // TransferObserver:
  void OnWillTransferAd(const AdInfo& ad, const base::Time time) override;
  void OnDidTransferAd(const AdInfo& ad) override;
  void OnCancelledTransfer(const AdInfo& ad, const int32_t tab_id) override;
  void OnFailedToTransferAd(const AdInfo& ad) override;

  // ConversionsObserver:
  void OnConversion(
      const ConversionQueueItemInfo& conversion_queue_item) override;

  bool is_initialized_ = false;

  std::unique_ptr<AdsClientHelper> ads_client_helper_;
  std::unique_ptr<Diagnostics> diagnostics_;
  std::unique_ptr<BrowserManager> browser_manager_;
  std::unique_ptr<TabManager> tab_manager_;
  std::unique_ptr<privacy::TokenGenerator> token_generator_;
  std::unique_ptr<Account> account_;
  std::unique_ptr<targeting::processor::EpsilonGreedyBandit>
      epsilon_greedy_bandit_processor_;
  std::unique_ptr<resource::EpsilonGreedyBandit>
      epsilon_greedy_bandit_resource_;
  std::unique_ptr<resource::TextClassification> text_classification_resource_;
  std::unique_ptr<targeting::processor::TextClassification>
      text_classification_processor_;
  std::unique_ptr<resource::PurchaseIntent> purchase_intent_resource_;
  std::unique_ptr<targeting::processor::PurchaseIntent>
      purchase_intent_processor_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<resource::Conversions> conversions_resource_;
  std::unique_ptr<targeting::geographic::SubdivisionTargeting>
      subdivision_targeting_;
  std::unique_ptr<ad_notifications::Serving> ad_notification_serving_;
  std::unique_ptr<AdNotification> ad_notification_;
  std::unique_ptr<AdNotifications> ad_notifications_;
  std::unique_ptr<AdServer> ad_server_;
  std::unique_ptr<Transfer> transfer_;
  std::unique_ptr<inline_content_ads::Serving> inline_content_ad_serving_;
  std::unique_ptr<InlineContentAd> inline_content_ad_;
  std::unique_ptr<Client> client_;
  std::unique_ptr<ConfirmationsState> confirmations_state_;
  std::unique_ptr<Conversions> conversions_;
  std::unique_ptr<database::Initialize> database_;
  std::unique_ptr<new_tab_page_ads::Serving> new_tab_page_ad_serving_;
  std::unique_ptr<NewTabPageAd> new_tab_page_ad_;
  std::unique_ptr<PromotedContentAd> promoted_content_ad_;
  std::unique_ptr<SearchResultAd> search_result_ad_;
  std::unique_ptr<UserActivity> user_activity_;
  std::unique_ptr<CovariateLogs> covariate_logs_;

  uint32_t last_html_loaded_hash_ = 0;
  uint32_t last_text_loaded_hash_ = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_IMPL_H_
