/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_IMPL_IOS_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_IMPL_IOS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier.h"
#include "brave/components/brave_ads/core/public/common/functional/once_closure_task_queue.h"

class PrefService;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_ads {

class Ads;
class AdsClient;
class AdsServiceImplIOS : public AdsService {
 public:
  explicit AdsServiceImplIOS(PrefService& prefs);

  AdsClientNotifier* GetAdsClientNotifier();

  AdsServiceImplIOS(const AdsServiceImplIOS&) = delete;
  AdsServiceImplIOS& operator=(const AdsServiceImplIOS&) = delete;

  ~AdsServiceImplIOS() override;

  void InitializeAds(const std::string& storage_path,
                     std::unique_ptr<AdsClient> ads_client,
                     mojom::SysInfoPtr mojom_sys_info,
                     mojom::BuildChannelInfoPtr mojom_build_channel,
                     mojom::WalletInfoPtr mojom_wallet,
                     ResultCallback callback);
  void ShutdownAds(ResultCallback callback);

  void MaybeGetNotificationAd(const std::string& placement_id,
                              MaybeGetNotificationAdCallback callback);
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      mojom::NotificationAdEventType mojom_ad_event_type,
      ResultCallback callback);

  void NotifyDidInitializeAdsService() const;
  void NotifyDidShutdownAdsService() const;
  void NotifyDidClearAdsServiceData() const;

  // AdsService:
  bool IsIneligibleToStart() const override;
  bool IsInitialized() const override;

  bool IsBrowserUpgradeRequiredToServeAds() const override;

  int64_t GetMaximumNotificationAdsPerHour() const override;

  void OnNotificationAdShown(const std::string& placement_id) override;
  void OnNotificationAdClosed(const std::string& placement_id,
                              bool by_user) override;
  void OnNotificationAdClicked(const std::string& placement_id) override;

  void ClearData(ResultCallback callback) override;

  void AddBatAdsObserver(mojo::PendingRemote<bat_ads::mojom::BatAdsObserver>
                             bat_ads_observer_pending_remote) override;

  void GetInternals(GetInternalsCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void ParseAndSaveNewTabPageAds(base::DictValue dict,
                                 ResultCallback callback) override;
  void MaybeServeNewTabPageAd(
      MaybeServeMojomNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdMetricType mojom_ad_metric_type,
      mojom::NewTabPageAdEventType mojom_ad_event_type,
      ResultCallback callback) override;

  void MaybeGetSearchResultAd(const std::string& placement_id,
                              MaybeGetSearchResultAdCallback callback) override;
  void TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      ResultCallback callback) override;

  void PurgeOrphanedAdEventsForType(mojom::AdType mojom_ad_type,
                                    ResultCallback callback) override;

  void GetAdHistory(base::Time from_time,
                    base::Time to_time,
                    GetAdHistoryForUICallback callback) override;

  void ToggleLikeAd(mojom::ReactionInfoPtr mojom_reaction,
                    ResultCallback callback) override;
  void ToggleDislikeAd(mojom::ReactionInfoPtr mojom_reaction,
                       ResultCallback callback) override;
  void ToggleLikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                         ResultCallback callback) override;
  void ToggleDislikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                            ResultCallback callback) override;
  void ToggleSaveAd(mojom::ReactionInfoPtr mojom_reaction,
                    ResultCallback callback) override;
  void ToggleMarkAdAsInappropriate(mojom::ReactionInfoPtr mojom_reaction,
                                   ResultCallback callback) override;

  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text) override;
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_new_navigation,
                          bool is_restoring,
                          bool is_visible) override;
  void NotifyTabDidLoad(int32_t tab_id, int http_status_code) override;
  void NotifyDidCloseTab(int32_t tab_id) override;

  void NotifyUserGestureEventTriggered(int32_t page_transition) override;

  void NotifyBrowserDidBecomeActive() override;
  void NotifyBrowserDidResignActive() override;

  void NotifyDidSolveAdaptiveCaptcha() override;

 private:
  // KeyedService:
  void Shutdown() override;

  void InitializeAds(ResultCallback callback);
  void InitializeAdsCallback(ResultCallback callback, bool success);

  void ShutdownAdsCallback(ResultCallback callback, bool success);

  void ClearAdsData(ResultCallback callback, bool success);
  void ClearAdsDataCallback(ResultCallback callback);

  const raw_ref<PrefService> prefs_;

  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  OnceClosureTaskQueue task_queue_;

  std::unique_ptr<AdsClientNotifier> ads_client_notifier_;

  base::FilePath storage_path_;
  std::unique_ptr<AdsClient> ads_client_;
  mojom::SysInfoPtr mojom_sys_info_;
  mojom::BuildChannelInfoPtr mojom_build_channel_;
  mojom::WalletInfoPtr mojom_wallet_;

  std::unique_ptr<Ads> ads_;

  base::WeakPtrFactory<AdsServiceImplIOS> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_IMPL_IOS_H_
