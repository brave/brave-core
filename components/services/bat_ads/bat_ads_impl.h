/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace ads {
class Ads;
struct InlineContentAdInfo;
struct NewTabPageAdInfo;
}  // namespace ads

namespace bat_ads {

class BatAdsClientMojoBridge;

class BatAdsImpl : public mojom::BatAds,
                   public base::SupportsWeakPtr<BatAdsImpl> {
 public:
  explicit BatAdsImpl(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient> client);

  BatAdsImpl(const BatAdsImpl&) = delete;
  BatAdsImpl& operator=(const BatAdsImpl&) = delete;

  BatAdsImpl(BatAdsImpl&& other) noexcept = delete;
  BatAdsImpl& operator=(BatAdsImpl&& other) noexcept = delete;

  ~BatAdsImpl() override;

  // mojom::BatAds:
  void Initialize(InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void OnLocaleDidChange(const std::string& locale) override;

  void OnPrefDidChange(const std::string& path) override;

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

  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  void OnTabDidStartPlayingMedia(int32_t tab_id) override;
  void OnTabDidStopPlayingMedia(int32_t tab_id) override;
  void OnTabDidChange(int32_t tab_id,
                      const std::vector<GURL>& redirect_chain,
                      bool is_active,
                      bool is_browser_active,
                      bool is_incognito) override;
  void OnDidCloseTab(int32_t tab_id) override;

  void OnRewardsWalletDidChange(const std::string& payment_id,
                                const std::string& seed) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      ads::mojom::InlineContentAdEventType event_type) override;

  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      ads::mojom::NewTabPageAdEventType event_type) override;

  void MaybeGetNotificationAd(const std::string& placement_id,
                              MaybeGetNotificationAdCallback callback) override;
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      ads::mojom::NotificationAdEventType event_type) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      ads::mojom::PromotedContentAdEventType event_type) override;

  void TriggerSearchResultAdEvent(
      ads::mojom::SearchResultAdInfoPtr ad_mojom,
      ads::mojom::SearchResultAdEventType event_type) override;

  void PurgeOrphanedAdEventsForType(
      ads::mojom::AdType ad_type,
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
  // TODO(https://github.com/brave/brave-browser/issues/24661) Workaround to
  // pass |base::OnceCallback| into |std::bind| until we refactor Brave Ads
  // |std::function| to |base::OnceCallback|.
  template <typename T>
  class CallbackHolder {
   public:
    CallbackHolder(base::WeakPtr<BatAdsImpl> client, T callback)
        : client_(std::move(client)), callback_(std::move(callback)) {}
    ~CallbackHolder() = default;

    bool is_valid() { return !!client_; }

    T& get() { return callback_; }

   private:
    base::WeakPtr<BatAdsImpl> client_;
    T callback_;
  };

  static void OnMaybeServeInlineContentAd(
      CallbackHolder<MaybeServeInlineContentAdCallback>* holder,
      const std::string& dimensions,
      const absl::optional<ads::InlineContentAdInfo>& ad);

  std::unique_ptr<BatAdsClientMojoBridge> bat_ads_client_mojo_proxy_;
  std::unique_ptr<ads::Ads> ads_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
