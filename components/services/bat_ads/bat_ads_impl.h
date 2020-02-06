/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bat/ads/ads.h"
#include "bat/ads/publisher_ads.h"

#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "base/memory/weak_ptr.h"

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

  // Overridden from mojom::BatAds:
  void Initialize(
      InitializeCallback callback) override;
  void Shutdown(
      ShutdownCallback callback) override;

  void SetConfirmationsIsReady(
      const bool is_ready) override;

  void ChangeLocale(
      const std::string& locale) override;

  void OnPageLoaded(
      const std::string& url,
      const std::string& html) override;

  void ServeSampleAd() override;

  void OnTimer(
      const uint32_t timer_id) override;

  void OnUnIdle() override;
  void OnIdle() override;

  void OnForeground() override;
  void OnBackground() override;

  void OnMediaPlaying(
      const int32_t tab_id) override;
  void OnMediaStopped(
      const int32_t tab_id) override;

  void OnTabUpdated(
      const int32_t tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_incognito) override;
  void OnTabClosed(
      const int32_t tab_id) override;

  void GetAdNotificationForId(
      const std::string& id,
      GetAdNotificationForIdCallback callback) override;
  void OnAdNotificationEvent(
      const std::string& id,
      const ads::AdNotificationEventType event_type) override;
  void OnPublisherAdEvent(
      const std::string& json,
      const ads::PublisherAdEventType event_type) override;

  void RemoveAllHistory(
      RemoveAllHistoryCallback callback) override;

  void GetAdsHistory(
      const uint64_t from_timestamp,
      const uint64_t to_timestamp,
      GetAdsHistoryCallback callback) override;

  void GetPublisherAds(
      const std::string& url,
      const std::vector<std::string>& sizes,
      GetPublisherAdsCallback callback) override;

  void ToggleAdThumbUp(
      const std::string& id,
      const std::string& creative_set_id,
      const int action,
      ToggleAdThumbUpCallback callback) override;
  void ToggleAdThumbDown(
      const std::string& id,
      const std::string& creative_set_id,
      const int action,
      ToggleAdThumbUpCallback callback) override;
  void ToggleAdOptInAction(
      const std::string& category,
      const int action,
      ToggleAdOptInActionCallback callback) override;
  void ToggleAdOptOutAction(
      const std::string& category,
      const int action,
      ToggleAdOptOutActionCallback callback) override;
  void ToggleSaveAd(
      const std::string& id,
      const std::string& creative_set_id,
      const bool saved,
      ToggleSaveAdCallback callback) override;
  void ToggleFlagAd(
      const std::string& id,
      const std::string& creative_set_id,
      const bool flagged,
      ToggleFlagAdCallback callback) override;

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

  static void OnInitialize(
      CallbackHolder<InitializeCallback>* holder,
      const int32_t result);

  static void OnShutdown(
      CallbackHolder<ShutdownCallback>* holder,
      const int32_t result);

  static void OnRemoveAllHistory(
      CallbackHolder<RemoveAllHistoryCallback>* holder,
      const int32_t result);

  static void OnGetPublisherAds(
      CallbackHolder<GetPublisherAdsCallback>* holder,
      const int32_t result,
      const std::string& url,
      const std::vector<std::string>& sizes,
      const ads::PublisherAds& ads);

  std::unique_ptr<BatAdsClientMojoBridge> bat_ads_client_mojo_proxy_;
  std::unique_ptr<ads::Ads> ads_;

  DISALLOW_COPY_AND_ASSIGN(BatAdsImpl);
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
