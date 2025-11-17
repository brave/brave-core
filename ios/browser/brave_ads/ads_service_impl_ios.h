/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_IMPL_IOS_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_IMPL_IOS_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/common/functional/once_closure_task_queue.h"
#include "brave/components/brave_ads/core/public/service/ads_service_callback.h"

class PrefService;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_ads {

class Ads;
class AdsClient;
struct NewTabPageAdInfo;
class NewTabPageAdPrefetcher;
class AdsServiceImplIOS : public AdsService {
 public:
  explicit AdsServiceImplIOS(PrefService* prefs);

  AdsServiceImplIOS(const AdsServiceImplIOS&) = delete;
  AdsServiceImplIOS& operator=(const AdsServiceImplIOS&) = delete;

  ~AdsServiceImplIOS() override;

  bool IsInitialized() const;

  void InitializeAds(const std::string& storage_path,
                     std::unique_ptr<AdsClient> ads_client,
                     mojom::SysInfoPtr mojom_sys_info,
                     mojom::BuildChannelInfoPtr mojom_build_channel,
                     mojom::WalletInfoPtr mojom_wallet,
                     InitializeCallback callback);
  void ShutdownAds(ShutdownCallback callback);

  void MaybeGetNotificationAd(const std::string& placement_id,
                              MaybeGetNotificationAdCallback callback);
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      mojom::NotificationAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  void NotifyDidInitializeAdsService() const;
  void NotifyDidShutdownAdsService() const;
  void NotifyDidClearAdsServiceData() const;

  // AdsService:
  bool IsBrowserUpgradeRequiredToServeAds() const override;

  int64_t GetMaximumNotificationAdsPerHour() const override;

  void OnNotificationAdShown(const std::string& placement_id) override;
  void OnNotificationAdClosed(const std::string& placement_id,
                              bool by_user) override;
  void OnNotificationAdClicked(const std::string& placement_id) override;

  void ClearData(ClearDataCallback callback) override;

  void AddBatAdsObserver(mojo::PendingRemote<bat_ads::mojom::BatAdsObserver>
                             bat_ads_observer_pending_remote) override;

  void GetInternals(GetInternalsCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  std::optional<NewTabPageAdInfo> MaybeGetPrefetchedNewTabPageAd() override;
  void PrefetchNewTabPageAd() override;
  void OnFailedToPrefetchNewTabPageAd(
      const std::string& placement_id,
      const std::string& creative_instance_id) override;
  void ParseAndSaveNewTabPageAds(
      base::Value::Dict dict,
      ParseAndSaveNewTabPageAdsCallback callback) override;
  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdMetricType mojom_ad_metric_type,
      mojom::NewTabPageAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  void MaybeGetSearchResultAd(const std::string& placement_id,
                              MaybeGetSearchResultAdCallback callback) override;
  void TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  void PurgeOrphanedAdEventsForType(
      mojom::AdType mojom_ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  void GetAdHistory(base::Time from_time,
                    base::Time to_time,
                    GetAdHistoryForUICallback callback) override;

  void ToggleLikeAd(mojom::ReactionInfoPtr mojom_reaction,
                    ToggleReactionCallback callback) override;
  void ToggleDislikeAd(mojom::ReactionInfoPtr mojom_reaction,
                       ToggleReactionCallback callback) override;
  void ToggleLikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                         ToggleReactionCallback callback) override;
  void ToggleDislikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                            ToggleReactionCallback callback) override;
  void ToggleSaveAd(mojom::ReactionInfoPtr mojom_reaction,
                    ToggleReactionCallback callback) override;
  void ToggleMarkAdAsInappropriate(mojom::ReactionInfoPtr mojom_reaction,
                                   ToggleReactionCallback callback) override;

  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text) override;
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html) override;
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_new_navigation,
                          bool is_restoring,
                          bool is_visible) override;
  void NotifyTabDidLoad(int32_t tab_id, int http_status_code) override;
  void NotifyDidCloseTab(int32_t tab_id) override;

  void NotifyUserGestureEventTriggered(int32_t page_transition_type) override;

  void NotifyBrowserDidBecomeActive() override;
  void NotifyBrowserDidResignActive() override;

  void NotifyDidSolveAdaptiveCaptcha() override;

 private:
  // KeyedService:
  void Shutdown() override;

  void InitializeAds(InitializeCallback callback);
  void InitializeAdsCallback(InitializeCallback callback, bool success);

  void ShutdownAdsCallback(ShutdownCallback callback, bool success);

  void ClearAdsData(ClearDataCallback callback, bool success);
  void ClearAdsDataCallback(ClearDataCallback callback);

  void RefetchNewTabPageAd();
  void RefetchNewTabPageAdCallback(bool success);
  void ResetNewTabPageAd();

  void OnParseAndSaveNewTabPageAdsCallback(
      ParseAndSaveNewTabPageAdsCallback callback,
      bool success);

  const raw_ptr<PrefService> prefs_;  // Not owned.

  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  OnceClosureTaskQueue task_queue_;

  base::FilePath storage_path_;
  std::unique_ptr<AdsClient> ads_client_;
  mojom::SysInfoPtr mojom_sys_info_;
  mojom::BuildChannelInfoPtr mojom_build_channel_;
  mojom::WalletInfoPtr mojom_wallet_;

  std::unique_ptr<NewTabPageAdPrefetcher> new_tab_page_ad_prefetcher_;

  std::unique_ptr<Ads> ads_;

  base::WeakPtrFactory<AdsServiceImplIOS> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_IMPL_IOS_H_
