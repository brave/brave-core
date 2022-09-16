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
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace ads {
class Ads;
struct NewTabPageAdInfo;
struct InlineContentAdInfo;
}  // namespace ads

namespace bat_ads {

class BatAdsClientMojoBridge;

class BatAdsImpl :
    public mojom::BatAds,
    public base::SupportsWeakPtr<BatAdsImpl> {
 public:
  explicit BatAdsImpl(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient> client);
  BatAdsImpl(const BatAdsImpl&) = delete;
  BatAdsImpl& operator=(const BatAdsImpl&) = delete;
  ~BatAdsImpl() override;

  // mojom::BatAds:
  void Initialize(InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void OnLocaleDidChange(const std::string& locale) override;

  void OnPrefDidChange(const std::string& path) override;

  void OnDidUpdateResourceComponent(const std::string& id) override;

  void OnTabHtmlContentDidChange(const int32_t tab_id,
                                 const std::vector<GURL>& redirect_chain,
                                 const std::string& html) override;
  void OnTabTextContentDidChange(const int32_t tab_id,
                                 const std::vector<GURL>& redirect_chain,
                                 const std::string& text) override;

  void OnUserDidBecomeIdle() override;
  void OnUserDidBecomeActive(const base::TimeDelta idle_time,
                             const bool screen_was_locked) override;

  void OnUserGesture(const int32_t page_transition_type) override;

  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  void OnMediaPlaying(const int32_t tab_id) override;
  void OnMediaStopped(const int32_t tab_id) override;

  void OnTabUpdated(const int32_t tab_id,
                    const std::vector<GURL>& redirect_chain,
                    const bool is_active,
                    const bool is_browser_active,
                    const bool is_incognito) override;
  void OnTabClosed(const int32_t tab_id) override;

  void OnWalletUpdated(const std::string& payment_id,
                       const std::string& seed) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::InlineContentAdEventType event_type) override;

  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::NewTabPageAdEventType event_type) override;

  void MaybeGetNotificationAd(const std::string& placement_id,
                              MaybeGetNotificationAdCallback callback) override;
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      const ads::mojom::NotificationAdEventType event_type) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::PromotedContentAdEventType event_type) override;

  void TriggerSearchResultAdEvent(
      ads::mojom::SearchResultAdInfoPtr ad_mojom,
      const ads::mojom::SearchResultAdEventType event_type) override;

  void PurgeOrphanedAdEventsForType(
      const ads::mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  void GetHistory(const base::Time from_time,
                  const base::Time to_time,
                  GetHistoryCallback callback) override;
  void RemoveAllHistory(RemoveAllHistoryCallback callback) override;

  void ToggleAdThumbUp(base::Value::Dict value,
                       ToggleAdThumbUpCallback callback) override;
  void ToggleAdThumbDown(base::Value::Dict value,
                         ToggleAdThumbUpCallback callback) override;
  void ToggleAdOptIn(const std::string& category,
                     const int opt_action_type,
                     ToggleAdOptInCallback callback) override;
  void ToggleAdOptOut(const std::string& category,
                      const int opt_action_type,
                      ToggleAdOptOutCallback callback) override;
  void ToggleSavedAd(base::Value::Dict value,
                     ToggleSavedAdCallback callback) override;
  void ToggleFlaggedAd(base::Value::Dict value,
                       ToggleFlaggedAdCallback callback) override;

 private:
  // TODO(https://github.com/brave/brave-browser/issues/20940) Workaround to
  // pass |base::OnceCallback| into |std::bind| until we refactor Brave Ads
  // |std::function| to |base::OnceCallback|.
  template <typename T>
  class CallbackHolder {
   public:
    CallbackHolder(base::WeakPtr<BatAdsImpl> client, T callback)
        : client_(client), callback_(std::move(callback)) {}
    ~CallbackHolder() = default;

    bool is_valid() { return !!client_.get(); }

    T& get() { return callback_; }

   private:
    base::WeakPtr<BatAdsImpl> client_;
    T callback_;
  };

  static void OnInitialize(CallbackHolder<InitializeCallback>* holder,
                           const bool success);
  static void OnShutdown(CallbackHolder<ShutdownCallback>* holder,
                         const bool success);

  static void OnGetDiagnostics(CallbackHolder<GetDiagnosticsCallback>* holder,
                               absl::optional<base::Value::List> value);

  static void OnGetStatementOfAccounts(
      CallbackHolder<GetStatementOfAccountsCallback>* holder,
      ads::mojom::StatementInfoPtr statement);

  static void OnMaybeServeInlineContentAd(
      CallbackHolder<MaybeServeInlineContentAdCallback>* holder,
      const std::string& dimensions,
      const absl::optional<ads::InlineContentAdInfo>& ad);

  static void OnMaybeServeNewTabPageAd(
      CallbackHolder<MaybeServeNewTabPageAdCallback>* holder,
      const absl::optional<ads::NewTabPageAdInfo>& ad);

  static void OnPurgeOrphanedAdEventsForType(
      CallbackHolder<PurgeOrphanedAdEventsForTypeCallback>* holder,
      const bool success);

  static void OnRemoveAllHistory(
      CallbackHolder<RemoveAllHistoryCallback>* holder,
      const bool success);

  std::unique_ptr<BatAdsClientMojoBridge> bat_ads_client_mojo_proxy_;
  std::unique_ptr<ads::Ads> ads_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
