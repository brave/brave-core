/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_TEST_FAKE_ADS_CLIENT_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_TEST_FAKE_ADS_CLIENT_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

namespace brave_ads::test {

// Fake no-op implementation of `AdsClient` for use in unit tests.
class FakeAdsClient final : public AdsClient {
 public:
  FakeAdsClient();

  FakeAdsClient(const FakeAdsClient&) = delete;
  FakeAdsClient& operator=(const FakeAdsClient&) = delete;

  ~FakeAdsClient() override;

  void AddObserver(AdsClientNotifierObserver* /*observer*/) override {}
  void RemoveObserver(AdsClientNotifierObserver* /*observer*/) override {}
  void NotifyPendingObservers() override {}

  bool IsNetworkConnectionAvailable() const override;

  bool IsBrowserActive() const override;
  bool IsBrowserInFullScreenMode() const override;

  bool CanShowNotificationAds() const override;
  bool CanShowNotificationAdsWhileBrowserIsBackgrounded() const override;
  void ShowNotificationAd(mojom::NotificationAdInfoPtr /*ad*/) override {}
  void CloseNotificationAd(const std::string& /*placement_id*/) override {}

  void GetSiteHistory(int max_count,
                      int recent_day_range,
                      GetSiteHistoryCallback callback) override;

  void UrlRequest(mojom::UrlRequestInfoPtr /*mojom_url_request*/,
                  UrlRequestCallback /*callback*/) override {}

  void Save(const std::string& name,
            const std::string& value,
            ResultCallback callback) override;
  void Remove(const std::string& name, ResultCallback callback) override;
  void Load(const std::string& name, LoadCallback callback) override;

  void LoadResourceComponent(const std::string& id,
                             int version,
                             LoadResourceComponentCallback callback) override;

  void ShowScheduledCaptcha(const std::string& /*payment_id*/,
                            const std::string& /*captcha_id*/) override {}

  bool FindProfilePref(const std::string& path) const override;
  std::optional<base::Value> GetProfilePref(const std::string& path) override;
  void SetProfilePref(const std::string& /*path*/,
                      base::Value /*value*/) override {}
  void ClearProfilePref(const std::string& /*path*/) override {}
  bool HasProfilePrefPath(const std::string& path) const override;

  bool FindLocalStatePref(const std::string& path) const override;
  std::optional<base::Value> GetLocalStatePref(
      const std::string& path) override;
  void SetLocalStatePref(const std::string& /*path*/,
                         base::Value /*value*/) override {}
  void ClearLocalStatePref(const std::string& /*path*/) override {}
  bool HasLocalStatePrefPath(const std::string& path) const override;

  base::DictValue GetVirtualPrefs() const override;

  void Log(const char* /*file*/,
           int /*line*/,
           int /*verbose_level*/,
           const std::string& /*message*/) override {}
};

}  // namespace brave_ads::test

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_TEST_FAKE_ADS_CLIENT_H_
