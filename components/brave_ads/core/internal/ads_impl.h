/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_handler.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/reminder/reminder.h"
#include "brave/components/brave_ads/core/internal/studies/studies.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_idle_detection/user_idle_detection.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/history/history_filter_types.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "brave/components/brave_ads/core/public/history/history_sort_types.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct NotificationAdInfo;

class AdsImpl final : public Ads {
 public:
  explicit AdsImpl(AdsClient* ads_client);

  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  AdsImpl(AdsImpl&&) noexcept = delete;
  AdsImpl& operator=(AdsImpl&&) noexcept = delete;

  ~AdsImpl() override;

  // Ads:
  void AddBatAdsObserver(
      std::unique_ptr<AdsObserverInterface> observer) override;

  void SetSysInfo(mojom::SysInfoPtr sys_info) override;
  void SetBuildChannel(mojom::BuildChannelInfoPtr build_channel) override;
  void SetFlags(mojom::FlagsPtr flags) override;

  void Initialize(mojom::WalletInfoPtr wallet,
                  InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(const std::string& placement_id,
                                   const std::string& creative_instance_id,
                                   mojom::InlineContentAdEventType event_type,
                                   TriggerAdEventCallback callback) override;

  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(const std::string& placement_id,
                                const std::string& creative_instance_id,
                                mojom::NewTabPageAdEventType event_type,
                                TriggerAdEventCallback callback) override;

  std::optional<NotificationAdInfo> MaybeGetNotificationAd(
      const std::string& placement_id) override;
  void TriggerNotificationAdEvent(const std::string& placement_id,
                                  mojom::NotificationAdEventType event_type,
                                  TriggerAdEventCallback callback) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType event_type,
      TriggerAdEventCallback callback) override;

  void TriggerSearchResultAdEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                                  mojom::SearchResultAdEventType event_type,
                                  TriggerAdEventCallback callback) override;

  void PurgeOrphanedAdEventsForType(
      mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  HistoryItemList GetHistory(HistoryFilterType filter_type,
                             HistorySortType sort_type,
                             base::Time from_time,
                             base::Time to_time) override;

  mojom::UserReactionType ToggleLikeAd(const base::Value::Dict& value) override;
  mojom::UserReactionType ToggleDislikeAd(
      const base::Value::Dict& value) override;
  mojom::UserReactionType ToggleLikeCategory(
      const base::Value::Dict& value) override;
  mojom::UserReactionType ToggleDislikeCategory(
      const base::Value::Dict& value) override;
  bool ToggleSaveAd(const base::Value::Dict& value) override;
  bool ToggleMarkAdAsInappropriate(const base::Value::Dict& value) override;

 private:
  void CreateOrOpenDatabase(mojom::WalletInfoPtr wallet,
                            InitializeCallback callback);
  void CreateOrOpenDatabaseCallback(mojom::WalletInfoPtr wallet,
                                    InitializeCallback callback,
                                    bool success);
  void PurgeExpiredAdEventsCallback(mojom::WalletInfoPtr wallet,
                                    InitializeCallback callback,
                                    bool success);
  void PurgeOrphanedAdEventsCallback(mojom::WalletInfoPtr wallet,
                                     InitializeCallback callback,
                                     bool success);
  void MigrateRewardsStateCallback(mojom::WalletInfoPtr wallet,
                                   InitializeCallback callback,
                                   bool success);
  void MigrateClientStateCallback(mojom::WalletInfoPtr wallet,
                                  InitializeCallback callback,
                                  bool success);
  void LoadClientStateCallback(mojom::WalletInfoPtr wallet,
                               InitializeCallback callback,
                               bool success);
  void MigrateConfirmationStateCallback(mojom::WalletInfoPtr wallet,
                                        InitializeCallback callback,
                                        bool success);
  void LoadConfirmationStateCallback(mojom::WalletInfoPtr wallet,
                                     InitializeCallback callback,
                                     bool success);
  void SuccessfullyInitialized(mojom::WalletInfoPtr wallet,
                               InitializeCallback callback);

  bool is_initialized_ = false;

  GlobalState global_state_;

  TokenGenerator token_generator_;
  Account account_;

  AdHandler ad_handler_;

  UserIdleDetection user_idle_detection_;

  Reactions reactions_;

  Reminder reminder_;

  Studies studies_;

  base::WeakPtrFactory<AdsImpl> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_
