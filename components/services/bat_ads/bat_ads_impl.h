/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_

#include <memory>
#include <string>

#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
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
  BatAdsImpl(mojo::PendingAssociatedRemote<mojom::BatAdsClient>
                 bat_ads_client_pending_associated_remote,
             mojo::PendingReceiver<mojom::BatAdsClientNotifier>
                 bat_ads_client_notifier_pending_receiver);

  BatAdsImpl(const BatAdsImpl&) = delete;
  BatAdsImpl& operator=(const BatAdsImpl&) = delete;

  BatAdsImpl(BatAdsImpl&& other) noexcept = delete;
  BatAdsImpl& operator=(BatAdsImpl&& other) noexcept = delete;

  ~BatAdsImpl() override;

  // mojom::BatAds:
  void AddBatAdsObserver(mojo::PendingRemote<mojom::BatAdsObserver>
                             bat_ads_observer_pending_remote) override;

  void SetSysInfo(brave_ads::mojom::SysInfoPtr mojom_sys_info) override;
  void SetBuildChannel(
      brave_ads::mojom::BuildChannelInfoPtr mojom_build_channel) override;
  void SetFlags(brave_ads::mojom::FlagsPtr mojom_flags) override;

  void Initialize(brave_ads::mojom::WalletInfoPtr mojom_wallet,
                  InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void GetInternals(GetInternalsCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      brave_ads::mojom::InlineContentAdEventType mojom_ad_event_type,
      TriggerInlineContentAdEventCallback callback) override;

  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
      TriggerNewTabPageAdEventCallback callback) override;

  void MaybeGetNotificationAd(const std::string& placement_id,
                              MaybeGetNotificationAdCallback callback) override;
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      brave_ads::mojom::NotificationAdEventType mojom_ad_event_type,
      TriggerNotificationAdEventCallback callback) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      brave_ads::mojom::PromotedContentAdEventType mojom_ad_event_type,
      TriggerPromotedContentAdEventCallback callback) override;

  void MaybeGetSearchResultAd(const std::string& placement_id,
                              MaybeGetSearchResultAdCallback callback) override;
  void TriggerSearchResultAdEvent(
      brave_ads::mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      brave_ads::mojom::SearchResultAdEventType mojom_ad_event_type,
      TriggerSearchResultAdEventCallback callback) override;

  void PurgeOrphanedAdEventsForType(
      brave_ads::mojom::AdType mojom_ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  void GetAdHistory(base::Time from_time,
                    base::Time to_time,
                    GetAdHistoryCallback callback) override;

  void ToggleLikeAd(brave_ads::mojom::ReactionInfoPtr reaction,
                    ToggleLikeAdCallback callback) override;
  void ToggleDislikeAd(brave_ads::mojom::ReactionInfoPtr reaction,
                       ToggleDislikeAdCallback callback) override;
  void ToggleLikeSegment(brave_ads::mojom::ReactionInfoPtr reaction,
                         ToggleLikeSegmentCallback callback) override;
  void ToggleDislikeSegment(brave_ads::mojom::ReactionInfoPtr reaction,
                            ToggleDislikeSegmentCallback callback) override;
  void ToggleSaveAd(brave_ads::mojom::ReactionInfoPtr reaction,
                    ToggleSaveAdCallback callback) override;
  void ToggleMarkAdAsInappropriate(
      brave_ads::mojom::ReactionInfoPtr reaction,
      ToggleMarkAdAsInappropriateCallback callback) override;

 private:
  brave_ads::Ads* GetAds();

  class AdsInstance;
  std::unique_ptr<AdsInstance, base::OnTaskRunnerDeleter> ads_instance_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
