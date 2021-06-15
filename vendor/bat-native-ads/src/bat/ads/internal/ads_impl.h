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
#include "bat/ads/internal/account/account_observer.h"
#include "bat/ads/internal/ad_server/ad_server_observer.h"
#include "bat/ads/internal/ad_transfer/ad_transfer_observer.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notification_observer.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_observer.h"
#include "bat/ads/internal/ads/promoted_content_ads/promoted_content_ad_observer.h"
#include "bat/ads/internal/conversions/conversions_observer.h"
#include "bat/ads/internal/privacy/tokens/token_generator.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_info.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

namespace ads {

namespace ad_notifications {
class AdServing;
}  // namespace ad_notifications

namespace ad_targeting {

namespace processor {
class EpsilonGreedyBandit;
class PurchaseIntent;
class TextClassification;
}  // namespace processor

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

}  // namespace ad_targeting

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

class Account;
class AdNotification;
class AdNotificationServing;
class AdNotifications;
class AdsClientHelper;
class AdServer;
class AdTargeting;
class AdTransfer;
class BrowserManager;
class Catalog;
class Client;
class ConfirmationsState;
class Conversions;
class NewTabPageAd;
class PromotedContentAd;
class TabManager;
class UserActivity;
struct AdInfo;
struct AdNotificationInfo;
struct AdsHistoryInfo;
struct NewTabPageAdInfo;
struct PromotedContentAdInfo;

class AdsImpl : public Ads,
                public AccountObserver,
                public AdNotificationObserver,
                public AdServerObserver,
                public AdTransferObserver,
                public ConversionsObserver,
                public NewTabPageAdObserver,
                public PromotedContentAdObserver {
 public:
  explicit AdsImpl(AdsClient* ads_client);

  ~AdsImpl() override;

  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  void set_for_testing(privacy::TokenGeneratorInterface* token_generator);

  bool IsInitialized();

  // Ads implementation
  void Initialize(InitializeCallback callback) override;

  void Shutdown(ShutdownCallback callback) override;

  void ChangeLocale(const std::string& locale) override;

  void OnPrefChanged(const std::string& path) override;

  void OnHtmlLoaded(const int32_t tab_id,
                    const std::vector<std::string>& redirect_chain,
                    const std::string& html) override;

  void OnTextLoaded(const int32_t tab_id,
                    const std::vector<std::string>& redirect_chain,
                    const std::string& text) override;

  void OnUserGesture(const int32_t page_transition_type) override;

  void OnIdle() override;
  void OnUnIdle(const int idle_time, const bool was_locked) override;

  void OnForeground() override;
  void OnBackground() override;

  void OnMediaPlaying(const int32_t tab_id) override;
  void OnMediaStopped(const int32_t tab_id) override;

  void OnTabUpdated(const int32_t tab_id,
                    const std::string& url,
                    const bool is_active,
                    const bool is_browser_active,
                    const bool is_incognito) override;

  void OnTabClosed(const int32_t tab_id) override;

  void OnWalletUpdated(const std::string& id, const std::string& seed) override;

  void OnResourceComponentUpdated(const std::string& id) override;

  bool GetAdNotification(const std::string& uuid,
                         AdNotificationInfo* ad_notification) override;
  void OnAdNotificationEvent(const std::string& uuid,
                             const AdNotificationEventType event_type) override;

  void OnNewTabPageAdEvent(const std::string& uuid,
                           const std::string& creative_instance_id,
                           const NewTabPageAdEventType event_type) override;

  void OnPromotedContentAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const PromotedContentAdEventType event_type) override;

  void RemoveAllHistory(RemoveAllHistoryCallback callback) override;

  void ReconcileAdRewards() override;

  AdsHistoryInfo GetAdsHistory(const AdsHistoryInfo::FilterType filter_type,
                               const AdsHistoryInfo::SortType sort_type,
                               const uint64_t from_timestamp,
                               const uint64_t to_timestamp) override;

  void GetAccountStatement(GetAccountStatementCallback callback) override;

  AdContentInfo::LikeAction ToggleAdThumbUp(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContentInfo::LikeAction& action) override;
  AdContentInfo::LikeAction ToggleAdThumbDown(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContentInfo::LikeAction& action) override;
  CategoryContentInfo::OptAction ToggleAdOptInAction(
      const std::string& category,
      const CategoryContentInfo::OptAction& action) override;
  CategoryContentInfo::OptAction ToggleAdOptOutAction(
      const std::string& category,
      const CategoryContentInfo::OptAction& action) override;
  bool ToggleSaveAd(const std::string& creative_instance_id,
                    const std::string& creative_set_id,
                    const bool saved) override;
  bool ToggleFlagAd(const std::string& creative_instance_id,
                    const std::string& creative_set_id,
                    const bool flagged) override;

 private:
  bool is_initialized_ = false;

  std::unique_ptr<AdsClientHelper> ads_client_helper_;
  std::unique_ptr<privacy::TokenGenerator> token_generator_;
  std::unique_ptr<Account> account_;
  std::unique_ptr<ad_targeting::processor::EpsilonGreedyBandit>
      epsilon_greedy_bandit_processor_;
  std::unique_ptr<resource::EpsilonGreedyBandit>
      epsilon_greedy_bandit_resource_;
  std::unique_ptr<resource::TextClassification> text_classification_resource_;
  std::unique_ptr<ad_targeting::processor::TextClassification>
      text_classification_processor_;
  std::unique_ptr<resource::PurchaseIntent> purchase_intent_resource_;
  std::unique_ptr<ad_targeting::processor::PurchaseIntent>
      purchase_intent_processor_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<resource::Conversions> conversions_resource_;
  std::unique_ptr<ad_targeting::geographic::SubdivisionTargeting>
      subdivision_targeting_;
  std::unique_ptr<AdTargeting> ad_targeting_;
  std::unique_ptr<ad_notifications::AdServing> ad_notification_serving_;
  std::unique_ptr<AdNotification> ad_notification_;
  std::unique_ptr<AdNotifications> ad_notifications_;
  std::unique_ptr<AdServer> ad_server_;
  std::unique_ptr<AdTransfer> ad_transfer_;
  std::unique_ptr<Client> client_;
  std::unique_ptr<Conversions> conversions_;
  std::unique_ptr<database::Initialize> database_;
  std::unique_ptr<NewTabPageAd> new_tab_page_ad_;
  std::unique_ptr<PromotedContentAd> promoted_content_ad_;
  std::unique_ptr<BrowserManager> browser_manager_;
  std::unique_ptr<TabManager> tab_manager_;
  std::unique_ptr<UserActivity> user_activity_;

  void set(privacy::TokenGeneratorInterface* token_generator);

  void InitializeBrowserManager();
  void InitializeDatabase(InitializeCallback callback);
  void MigrateConversions(InitializeCallback callback);
  void LoadClientState(InitializeCallback callback);
  void LoadConfirmationsState(InitializeCallback callback);
  void LoadAdNotificationsState(InitializeCallback callback);
  void Initialized(InitializeCallback callback);
  void Start();

  void CleanupAdEvents();

  void MaybeUpdateCatalog();

  void MaybeServeAdNotification();
  void MaybeServeAdNotificationsAtRegularIntervals();

  // AccountObserver implementation
  void OnAdRewardsChanged() override;
  void OnTransactionsChanged() override;

  // AdServerObserver implementation
  void OnCatalogUpdated(const Catalog& catalog) override;

  // AdNotificationObserver implementation
  void OnAdNotificationViewed(const AdNotificationInfo& ad) override;
  void OnAdNotificationClicked(const AdNotificationInfo& ad) override;
  void OnAdNotificationDismissed(const AdNotificationInfo& ad) override;
  void OnAdNotificationTimedOut(const AdNotificationInfo& ad) override;

  // NewTabPageAdObserver implementation
  void OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) override;
  void OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) override;

  // PromotedContentAdObserver implementation
  void OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) override;
  void OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) override;

  // AdTransferObserver implementation
  void OnAdTransfer(const AdInfo& ad) override;

  // ConversionsObserver implementation
  void OnConversion(
      const ConversionQueueItemInfo& conversion_queue_item) override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_IMPL_H_
