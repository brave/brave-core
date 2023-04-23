/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/ads.h"
#include "brave/components/brave_ads/core/history_filter_types.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/history_sort_types.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/account/account_observer.h"
#include "brave/components/brave_ads/core/internal/ads/inline_content_ad_handler.h"
#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_handler.h"
#include "brave/components/brave_ads/core/internal/ads/notification_ad_handler.h"
#include "brave/components/brave_ads/core/internal/ads/promoted_content_ad_handler.h"
#include "brave/components/brave_ads/core/internal/ads/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_observer.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_processor.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/purchase_intent/purchase_intent_processor.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_classification/text_classification_processor.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_processor.h"
#include "brave/components/brave_ads/core/internal/reminder/reminder.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/multi_armed_bandits/epsilon_greedy_bandit_resource.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/text_embedding_resource.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer_observer.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_reactions/user_reactions.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

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
  void SetSysInfo(mojom::SysInfoPtr sys_info) override;
  void SetBuildChannel(mojom::BuildChannelInfoPtr build_channel) override;
  void SetFlags(mojom::FlagsPtr flags) override;

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
  CategoryContentOptActionType ToggleLikeCategory(
      const std::string& category,
      const CategoryContentOptActionType& action_type) override;
  CategoryContentOptActionType ToggleDislikeCategory(
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

  GlobalState global_state_;

  Catalog catalog_;

  privacy::TokenGenerator token_generator_;
  Account account_;

  Transfer transfer_;

  Conversions conversions_;

  SubdivisionTargeting subdivision_targeting_;

  resource::AntiTargeting anti_targeting_resource_;
  resource::EpsilonGreedyBandit epsilon_greedy_bandit_resource_;
  resource::PurchaseIntent purchase_intent_resource_;
  resource::TextClassification text_classification_resource_;
  resource::TextEmbedding text_embedding_resource_;

  processor::EpsilonGreedyBandit epsilon_greedy_bandit_processor_;
  processor::PurchaseIntent purchase_intent_processor_;
  processor::TextClassification text_classification_processor_;
  processor::TextEmbedding text_embedding_processor_;

  InlineContentAdHandler inline_content_ad_handler_;
  NewTabPageAdHandler new_tab_page_ad_handler_;
  NotificationAdHandler notification_ad_handler_;
  PromotedContentAd promoted_content_ad_handler_;
  SearchResultAd search_result_ad_handler_;

  Reminder reminder_;

  UserReactions user_reactions_;

  base::WeakPtrFactory<AdsImpl> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_
