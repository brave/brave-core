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

#include "bat/ads/ads.h"

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
  explicit BatAdsImpl(mojom::BatAdsClientAssociatedPtrInfo client_info);
  ~BatAdsImpl() override;

  // Overridden from mojom::BatAds:
  void Initialize(InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;
  void SetConfirmationsIsReady(const bool is_ready) override;
  void ChangeLocale(const std::string& locale) override;
  void ClassifyPage(const std::string& url, const std::string& page) override;
  void ServeSampleAd() override;
  void OnTimer(const uint32_t timer_id) override;
  void OnUnIdle() override;
  void OnIdle() override;
  void OnForeground() override;
  void OnBackground() override;
  void OnMediaPlaying(const int32_t tab_id) override;
  void OnMediaStopped(const int32_t tab_id) override;
  void OnTabUpdated(
      const int32_t tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_incognito) override;
  void OnTabClosed(const int32_t tab_id) override;
  void GetNotificationForId(
      const std::string& id,
      GetNotificationForIdCallback callback) override;
  void OnNotificationEvent(const std::string& id, const int32_t type) override;
  void RemoveAllHistory(RemoveAllHistoryCallback callback) override;

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

  std::unique_ptr<BatAdsClientMojoBridge> bat_ads_client_mojo_proxy_;
  std::unique_ptr<ads::Ads> ads_;

  DISALLOW_COPY_AND_ASSIGN(BatAdsImpl);
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
