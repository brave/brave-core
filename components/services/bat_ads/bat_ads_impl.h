/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"

namespace brave_ads {
class Ads;
struct InlineContentAdInfo;
struct NewTabPageAdInfo;
}  // namespace brave_ads

namespace bat_ads {

class BatAdsClientMojoBridge;

class BatAdsImpl : public mojom::BatAds {
 public:
  BatAdsImpl(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient> client,
      mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier);

  BatAdsImpl(const BatAdsImpl&) = delete;
  BatAdsImpl& operator=(const BatAdsImpl&) = delete;

  BatAdsImpl(BatAdsImpl&& other) noexcept = delete;
  BatAdsImpl& operator=(BatAdsImpl&& other) noexcept = delete;

  ~BatAdsImpl() override;

  // mojom::BatAds:
  void SetSysInfo(brave_ads::mojom::SysInfoPtr sys_info) override;
  void SetBuildChannel(
      brave_ads::mojom::BuildChannelInfoPtr build_channel) override;
  void SetFlags(brave_ads::mojom::FlagsPtr flags) override;

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
      brave_ads::mojom::InlineContentAdEventType event_type) override;

  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      brave_ads::mojom::NewTabPageAdEventType event_type) override;

  void MaybeGetNotificationAd(const std::string& placement_id,
                              MaybeGetNotificationAdCallback callback) override;
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      brave_ads::mojom::NotificationAdEventType event_type) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      brave_ads::mojom::PromotedContentAdEventType event_type) override;

  void TriggerSearchResultAdEvent(
      brave_ads::mojom::SearchResultAdInfoPtr ad_mojom,
      brave_ads::mojom::SearchResultAdEventType event_type) override;

  void PurgeOrphanedAdEventsForType(
      brave_ads::mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  void GetHistory(base::Time from_time,
                  base::Time to_time,
                  GetHistoryCallback callback) override;
  void RemoveAllHistory(RemoveAllHistoryCallback callback) override;

  void ToggleLikeAd(base::Value::Dict value,
                    ToggleLikeAdCallback callback) override;
  void ToggleDislikeAd(base::Value::Dict value,
                       ToggleLikeAdCallback callback) override;
  void ToggleLikeCategory(const std::string& category,
                          brave_ads::mojom::UserReactionType user_reaction_type,
                          ToggleLikeCategoryCallback callback) override;
  void ToggleDislikeCategory(
      const std::string& category,
      brave_ads::mojom::UserReactionType user_reaction_type,
      ToggleDislikeCategoryCallback callback) override;
  void ToggleSaveAd(base::Value::Dict value,
                    ToggleSaveAdCallback callback) override;
  void ToggleMarkAdAsInappropriate(
      base::Value::Dict value,
      ToggleMarkAdAsInappropriateCallback callback) override;

 private:
  brave_ads::Ads* GetAds();

  class AdsInstance;
  std::unique_ptr<AdsInstance, base::OnTaskRunnerDeleter> ads_instance_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
