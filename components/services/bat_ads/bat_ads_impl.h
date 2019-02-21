/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_

#include <memory>
#include <string>

#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace ads {
class Ads;
}

namespace bat_ads {

class BatAdsClientMojoBridge;

class BatAdsImpl : public mojom::BatAds {
 public:
  explicit BatAdsImpl(mojom::BatAdsClientAssociatedPtrInfo client_info);
  ~BatAdsImpl() override;

  // Overridden from mojom::BatAds:
  void Initialize(InitializeCallback callback) override;
  void ClassifyPage(const std::string& url,
                    const std::string& page) override;
  void TabClosed(int32_t tab_id) override;
  void OnTimer(uint32_t timer_id) override;
  void OnUnIdle() override;
  void OnIdle() override;
  void OnForeground() override;
  void OnBackground() override;
  void OnMediaPlaying(int32_t tab_id) override;
  void OnMediaStopped(int32_t tab_id) override;
  void TabUpdated(int32_t tab_id,
                  const std::string& url,
                  bool is_active,
                  bool is_incognito) override;
  void RemoveAllHistory(RemoveAllHistoryCallback callback) override;
  void SetConfirmationsIsReady(const bool is_ready) override;
  void ServeSampleAd() override;
  void GenerateAdReportingNotificationShownEvent(
      const std::string& notification_info) override;
  void GenerateAdReportingNotificationResultEvent(
      const std::string& notification_info,
      int32_t event_type) override;

 private:
  std::unique_ptr<BatAdsClientMojoBridge> bat_ads_client_mojo_proxy_;
  std::unique_ptr<ads::Ads> ads_;

  DISALLOW_COPY_AND_ASSIGN(BatAdsImpl);
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_IMPL_H_
