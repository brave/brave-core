/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_IMPL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_IMPL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "bat/ads/ads.h"
#include "bat/ads/history_filter_types.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/history_sort_types.h"
#include "bat/ads/internal/conversions/conversions_observer.h"
#include "bat/ads/internal/database/database_manager_observer.h"
#include "bat/ads/internal/transfer/transfer_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace privacy {
class TokenGenerator;
}  // namespace privacy

namespace processor {
class EpsilonGreedyBandit;
class PurchaseIntent;
class TextClassification;
class TextEmbedding;
}  // namespace processor

namespace resource {
class AntiTargeting;
class EpsilonGreedyBandit;
class PurchaseIntent;
class TextClassification;
class TextEmbedding;
}  // namespace resource

class Account;
class AdsClientHelper;
class AdsObserverManager;
class BrowserManager;
class Catalog;
class ClientStateManager;
class ConfirmationStateManager;
class Conversions;
class CovariateManager;
class DatabaseManager;
class DiagnosticManager;
class FlagManager;
class HistoryManager;
class IdleDetection;
class InlineContentAd;
class NewTabPageAd;
class NotificationAd;
class NotificationAdManager;
class PromotedContentAd;
class SearchResultAd;
class TabManager;
class Transfer;
class UserActivityManager;
class UserReactions;
struct AdInfo;
struct ConversionQueueItemInfo;
struct NotificationAdInfo;

class AdsImpl final : public Ads,
                      public ConversionsObserver,
                      public DatabaseManagerObserver,
                      public TransferObserver {
 public:
  explicit AdsImpl(AdsClient* ads_client);

  AdsImpl(const AdsImpl& other) = delete;
  AdsImpl& operator=(const AdsImpl& other) = delete;

  AdsImpl(AdsImpl&& other) noexcept = delete;
  AdsImpl& operator=(AdsImpl&& other) noexcept = delete;

  ~AdsImpl() override;

  // Ads:
  void AddBatAdsObserver(
      mojo::PendingRemote<bat_ads::mojom::BatAdsObserver> observer) override;

  void Initialize(InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void TriggerUserGestureEvent(int32_t page_transition_type) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType event_type) override;

  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdEventType event_type) override;

  absl::optional<NotificationAdInfo> MaybeGetNotificationAd(
      const std::string& placement_id) override;
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      mojom::NotificationAdEventType event_type) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType event_type) override;

  void TriggerSearchResultAdEvent(
      mojom::SearchResultAdInfoPtr ad_mojom,
      mojom::SearchResultAdEventType event_type) override;

  void PurgeOrphanedAdEventsForType(
      mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  HistoryItemList GetHistory(HistoryFilterType filter_type,
                             HistorySortType sort_type,
                             base::Time from_time,
                             base::Time to_time) override;
  void RemoveAllHistory(RemoveAllHistoryCallback callback) override;

  AdContentLikeActionType ToggleAdThumbUp(base::Value::Dict value) override;
  AdContentLikeActionType ToggleAdThumbDown(base::Value::Dict value) override;
  CategoryContentOptActionType ToggleAdOptIn(
      const std::string& category,
      const CategoryContentOptActionType& action_type) override;
  CategoryContentOptActionType ToggleAdOptOut(
      const std::string& category,
      const CategoryContentOptActionType& action_type) override;
  bool ToggleSavedAd(base::Value::Dict value) override;
  bool ToggleFlaggedAd(base::Value::Dict value) override;

 private:
  void CreateOrOpenDatabase(InitializeCallback callback);
  void OnCreateOrOpenDatabase(InitializeCallback callback, bool success);
  void OnMigrateConversions(InitializeCallback callback, bool success);
  void OnMigrateRewards(InitializeCallback callback, bool success);
  void OnMigrateClientState(InitializeCallback callback, bool success);
  void OnLoadClientState(InitializeCallback callback, bool success);
  void OnMigrateConfirmationState(InitializeCallback callback, bool success);
  void OnLoadConfirmationState(InitializeCallback callback, bool success);
  void OnMigrateNotificationState(InitializeCallback callback, bool success);

  bool IsInitialized() const;

  void Start();

  // ConversionsObserver:
  void OnConversion(
      const ConversionQueueItemInfo& conversion_queue_item) override;

  // DatabaseManagerObserver:
  void OnDatabaseIsReady() override;

  // TransferObserver:
  void OnDidTransferAd(const AdInfo& ad) override;

  bool is_initialized_ = false;

  std::unique_ptr<AdsClientHelper> ads_client_helper_;

  std::unique_ptr<AdsObserverManager> ads_observer_manager_;
  std::unique_ptr<BrowserManager> browser_manager_;
  std::unique_ptr<ClientStateManager> client_state_manager_;
  std::unique_ptr<FlagManager> flag_manager_;
  std::unique_ptr<ConfirmationStateManager> confirmation_state_manager_;
  std::unique_ptr<CovariateManager> covariate_manager_;
  std::unique_ptr<DatabaseManager> database_manager_;
  std::unique_ptr<DiagnosticManager> diagnostic_manager_;
  std::unique_ptr<HistoryManager> history_manager_;
  std::unique_ptr<NotificationAdManager> notification_ad_manager_;
  std::unique_ptr<TabManager> tab_manager_;
  std::unique_ptr<UserActivityManager> user_activity_manager_;

  std::unique_ptr<IdleDetection> idle_detection_;

  std::unique_ptr<Catalog> catalog_;

  std::unique_ptr<privacy::TokenGenerator> token_generator_;
  std::unique_ptr<Account> account_;

  std::unique_ptr<Transfer> transfer_;

  std::unique_ptr<Conversions> conversions_;

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;

  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<resource::EpsilonGreedyBandit>
      epsilon_greedy_bandit_resource_;
  std::unique_ptr<resource::PurchaseIntent> purchase_intent_resource_;
  std::unique_ptr<resource::TextClassification> text_classification_resource_;
  std::unique_ptr<resource::TextEmbedding> text_embedding_resource_;

  std::unique_ptr<processor::EpsilonGreedyBandit>
      epsilon_greedy_bandit_processor_;
  std::unique_ptr<processor::PurchaseIntent> purchase_intent_processor_;
  std::unique_ptr<processor::TextClassification> text_classification_processor_;
  std::unique_ptr<processor::TextEmbedding> text_embedding_processor_;

  std::unique_ptr<InlineContentAd> inline_content_ad_;
  std::unique_ptr<NewTabPageAd> new_tab_page_ad_;
  std::unique_ptr<NotificationAd> notification_ad_;
  std::unique_ptr<PromotedContentAd> promoted_content_ad_;
  std::unique_ptr<SearchResultAd> search_result_ad_;

  std::unique_ptr<UserReactions> user_reactions_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_IMPL_H_
