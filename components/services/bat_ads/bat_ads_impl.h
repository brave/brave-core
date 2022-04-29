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
#include "bat/ads/ads.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "bat/ads/statement_info.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"

namespace ads {
class Ads;
}

namespace bat_ads {

class BatAdsClientMojoBridge;

class BatAdsImpl :
    public mojom::BatAds,
    public base::SupportsWeakPtr<BatAdsImpl> {
 public:
  explicit BatAdsImpl(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info);
  ~BatAdsImpl() override;

  BatAdsImpl(const BatAdsImpl&) = delete;
  BatAdsImpl& operator=(const BatAdsImpl&) = delete;

  // Overridden from mojom::BatAds:
  void Initialize(
      InitializeCallback callback) override;
  void Shutdown(
      ShutdownCallback callback) override;

  void ChangeLocale(
      const std::string& locale) override;

  void OnPrefChanged(const std::string& path) override;

  void OnHtmlLoaded(const int32_t tab_id,
                    const std::vector<GURL>& redirect_chain,
                    const std::string& html) override;

  void OnTextLoaded(const int32_t tab_id,
                    const std::vector<GURL>& redirect_chain,
                    const std::string& text) override;

  void OnUserGesture(const int32_t page_transition_type) override;

  void OnUnIdle(const int idle_time, const bool was_locked) override;
  void OnIdle() override;

  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  void OnMediaPlaying(
      const int32_t tab_id) override;
  void OnMediaStopped(
      const int32_t tab_id) override;

  void OnTabUpdated(const int32_t tab_id,
                    const GURL& url,
                    const bool is_active,
                    const bool is_browser_active,
                    const bool is_incognito) override;
  void OnTabClosed(
      const int32_t tab_id) override;

  void GetAdNotification(const std::string& placement_id,
                         GetAdNotificationCallback callback) override;
  void TriggerAdNotificationEvent(
      const std::string& placement_id,
      const ads::mojom::AdNotificationEventType event_type) override;

  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::NewTabPageAdEventType event_type) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::PromotedContentAdEventType event_type) override;

  void GetInlineContentAd(const std::string& dimensions,
                          GetInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::InlineContentAdEventType event_type) override;

  void TriggerSearchResultAdEvent(
      ads::mojom::SearchResultAdPtr ad_mojom,
      const ads::mojom::SearchResultAdEventType event_type,
      TriggerSearchResultAdEventCallback callback) override;

  void PurgeOrphanedAdEventsForType(const ads::mojom::AdType ad_type) override;

  void RemoveAllHistory(
      RemoveAllHistoryCallback callback) override;

  void OnWalletUpdated(
      const std::string& payment_id,
      const std::string& seed) override;

  void GetHistory(const base::Time from_time,
                  const base::Time to_time,
                  GetHistoryCallback callback) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void ToggleAdThumbUp(const std::string& json,
                       ToggleAdThumbUpCallback callback) override;
  void ToggleAdThumbDown(const std::string& json,
                         ToggleAdThumbUpCallback callback) override;

  void ToggleAdOptIn(const std::string& category,
                     const int opt_action_type,
                     ToggleAdOptInCallback callback) override;
  void ToggleAdOptOut(const std::string& category,
                      const int opt_action_type,
                      ToggleAdOptOutCallback callback) override;

  void ToggleSavedAd(const std::string& json,
                     ToggleSavedAdCallback callback) override;

  void ToggleFlaggedAd(const std::string& json,
                       ToggleFlaggedAdCallback callback) override;

  void OnResourceComponentUpdated(const std::string& id) override;

 private:
  // Workaround to pass base::OnceCallback into std::bind
  template <typename Callback>
    class CallbackHolder {
     public:
      CallbackHolder(base::WeakPtr<BatAdsImpl> client, Callback callback) :
          client_(client),
          callback_(std::move(callback)) {}
      ~CallbackHolder() = default;

      bool is_valid() {
        return !!client_.get();
      }

      Callback& get() {
        return callback_;
      }

     private:
      base::WeakPtr<BatAdsImpl> client_;
      Callback callback_;
    };

    static void OnInitialize(CallbackHolder<InitializeCallback>* holder,
                             const bool success);

    static void OnShutdown(CallbackHolder<ShutdownCallback>* holder,
                           const bool success);

    static void OnGetInlineContentAd(
        CallbackHolder<GetInlineContentAdCallback>* holder,
        const bool success,
        const std::string& dimensions,
        const ads::InlineContentAdInfo& ad);

    static void OnTriggerSearchResultAdEvent(
        CallbackHolder<TriggerSearchResultAdEventCallback>* holder,
        const bool success,
        const std::string& placement_id,
        const ads::mojom::SearchResultAdEventType event_type);

    static void OnRemoveAllHistory(
        CallbackHolder<RemoveAllHistoryCallback>* holder,
        const bool success);

    static void OnGetStatementOfAccounts(
        CallbackHolder<GetStatementOfAccountsCallback>* holder,
        const bool success,
        const ads::StatementInfo& statement);

    static void OnGetDiagnostics(CallbackHolder<GetDiagnosticsCallback>* holder,
                                 const bool success,
                                 const std::string& json);

    std::unique_ptr<BatAdsClientMojoBridge> bat_ads_client_mojo_proxy_;
    std::unique_ptr<ads::Ads> ads_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
