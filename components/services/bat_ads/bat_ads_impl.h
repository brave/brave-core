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

#include "base/values.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"

class GURL;

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
  void Initialize(InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void OnDidUpdateResourceComponent(const std::string& id) override;

  void OnTabHtmlContentDidChange(int32_t tab_id,
                                 const std::vector<GURL>& redirect_chain,
                                 const std::string& html) override;
  void OnTabTextContentDidChange(int32_t tab_id,
                                 const std::vector<GURL>& redirect_chain,
                                 const std::string& text) override;

  void OnUserDidBecomeIdle() override;
  void OnUserDidBecomeActive(base::TimeDelta idle_time,
                             bool screen_was_locked) override;

  void TriggerUserGestureEvent(int32_t page_transition_type) override;

  void OnTabDidStartPlayingMedia(int32_t tab_id) override;
  void OnTabDidStopPlayingMedia(int32_t tab_id) override;
  void OnTabDidChange(int32_t tab_id,
                      const std::vector<GURL>& redirect_chain,
                      bool is_active,
                      bool is_browser_active,
                      bool is_incognito) override;
  void OnDidCloseTab(int32_t tab_id) override;

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

  void ToggleAdThumbUp(base::Value::Dict value,
                       ToggleAdThumbUpCallback callback) override;
  void ToggleAdThumbDown(base::Value::Dict value,
                         ToggleAdThumbUpCallback callback) override;
  void ToggleAdOptIn(const std::string& category,
                     int opt_action_type,
                     ToggleAdOptInCallback callback) override;
  void ToggleAdOptOut(const std::string& category,
                      int opt_action_type,
                      ToggleAdOptOutCallback callback) override;
  void ToggleSavedAd(base::Value::Dict value,
                     ToggleSavedAdCallback callback) override;
  void ToggleFlaggedAd(base::Value::Dict value,
                       ToggleFlaggedAdCallback callback) override;

 private:
  std::unique_ptr<BatAdsClientMojoBridge> bat_ads_client_mojo_proxy_;
  std::unique_ptr<brave_ads::Ads> ads_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
