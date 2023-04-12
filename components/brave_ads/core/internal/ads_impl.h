/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/ads.h"
#include "brave/components/brave_ads/core/history_filter_types.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/history_sort_types.h"
#include "brave/components/brave_ads/core/internal/account/account_observer.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_observer.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer_observer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

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
class GlobalState;
class Catalog;
class Conversions;
class IdleDetection;
class InlineContentAdHandler;
class NewTabPageAdHandler;
class NotificationAdHandler;
class PromotedContentAd;
class Reminder;
class SearchResultAd;
class SubdivisionTargeting;
class TabManager;
class Transfer;
class UserReactions;
struct AdInfo;
struct ConversionQueueItemInfo;
struct NotificationAdInfo;

class AdsImpl final : public Ads,
                      public AccountObserver,
                      public ConversionsObserver,
                      public TransferObserver {
 public:
  explicit AdsImpl(AdsClient* ads_client);

  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  AdsImpl(AdsImpl&&) noexcept = delete;
  AdsImpl& operator=(AdsImpl&&) noexcept = delete;

  ~AdsImpl() override;

  // Ads:
  void Initialize(InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void OnRewardsWalletDidChange(const std::string& payment_id,
                                const std::string& recovery_seed) override;

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

  AdContentLikeActionType ToggleLikeAd(base::Value::Dict value) override;
  AdContentLikeActionType ToggleDislikeAd(base::Value::Dict value) override;
  CategoryContentOptActionType ToggleMarkToReceiveAdsForCategory(
      const std::string& category,
      const CategoryContentOptActionType& action_type) override;
  CategoryContentOptActionType ToggleMarkToNoLongerReceiveAdsForCategory(
      const std::string& category,
      const CategoryContentOptActionType& action_type) override;
  bool ToggleSaveAd(base::Value::Dict value) override;
  bool ToggleMarkAdAsInappropriate(base::Value::Dict value) override;

 private:
  void CreateOrOpenDatabase(InitializeCallback callback);
  void OnCreateOrOpenDatabase(InitializeCallback callback, bool success);
  void OnPurgeExpiredAdEvents(InitializeCallback callback, bool success);
  void OnPurgeOrphanedAdEvents(InitializeCallback callback, bool success);
  void OnMigrateConversions(InitializeCallback callback, bool success);
  void OnMigrateRewards(InitializeCallback callback, bool success);
  void OnMigrateClientState(InitializeCallback callback, bool success);
  void OnLoadClientState(InitializeCallback callback, bool success);
  void OnMigrateConfirmationState(InitializeCallback callback, bool success);
  void OnLoadConfirmationState(InitializeCallback callback, bool success);
  void OnMigrateNotificationState(InitializeCallback callback, bool success);

  bool IsInitialized() const { return is_initialized_; }

  void Start();

  // AccountObserver:
  void OnStatementOfAccountsDidChange() override;

  // ConversionsObserver:
  void OnConversion(
      const ConversionQueueItemInfo& conversion_queue_item) override;

  // TransferObserver:
  void OnDidTransferAd(const AdInfo& ad) override;

  bool is_initialized_ = false;

  std::unique_ptr<GlobalState> global_state_;

  std::unique_ptr<Catalog> catalog_;

  std::unique_ptr<privacy::TokenGenerator> token_generator_;
  std::unique_ptr<Account> account_;

  std::unique_ptr<Transfer> transfer_;

  std::unique_ptr<Conversions> conversions_;

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;

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

  std::unique_ptr<InlineContentAdHandler> inline_content_ad_handler_;
  std::unique_ptr<NewTabPageAdHandler> new_tab_page_ad_handler_;
  std::unique_ptr<NotificationAdHandler> notification_ad_handler_;
  std::unique_ptr<PromotedContentAd> promoted_content_ad_handler_;
  std::unique_ptr<SearchResultAd> search_result_ad_handler_;

  std::unique_ptr<Reminder> reminder_;

  std::unique_ptr<UserReactions> user_reactions_;

  base::WeakPtrFactory<AdsImpl> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_
