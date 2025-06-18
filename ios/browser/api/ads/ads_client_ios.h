/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_IOS_H_
#define BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_IOS_H_

#import <Foundation/Foundation.h>

#include <optional>
#include <string>
#include <vector>

#import "brave/components/brave_ads/core/public/ads_client/ads_client.h"

@protocol AdsClientBridge;

class AdsClientIOS : public brave_ads::AdsClient {
 public:
  explicit AdsClientIOS(id<AdsClientBridge> bridge);
  ~AdsClientIOS() override;

 private:
  __weak id<AdsClientBridge> bridge_;

  void AddObserver(brave_ads::AdsClientNotifierObserver* observer) override;
  void RemoveObserver(brave_ads::AdsClientNotifierObserver* observer) override;
  void NotifyPendingObservers() override;
  bool IsNetworkConnectionAvailable() const override;
  bool IsBrowserActive() const override;
  bool IsBrowserInFullScreenMode() const override;
  bool CanShowNotificationAdsWhileBrowserIsBackgrounded() const override;
  void ShowNotificationAd(const brave_ads::NotificationAdInfo& ad) override;
  bool CanShowNotificationAds() const override;
  void CloseNotificationAd(const std::string& placement_id) override;
  base::Value::Dict GetVirtualPrefs() const override;
  void UrlRequest(brave_ads::mojom::UrlRequestInfoPtr mojom_url_request,
                  brave_ads::UrlRequestCallback callback) override;
  void Save(const std::string& name,
            const std::string& value,
            brave_ads::SaveCallback callback) override;
  void Load(const std::string& name, brave_ads::LoadCallback callback) override;
  void LoadResourceComponent(const std::string& id,
                             int version,
                             brave_ads::LoadFileCallback callback) override;
  void GetSiteHistory(int max_count,
                      int days_ago,
                      brave_ads::GetSiteHistoryCallback callback) override;
  void ShowScheduledCaptcha(const std::string& payment_id,
                            const std::string& captcha_id) override;
  void Log(const char* file,
           int line,
           int verbose_level,
           const std::string& message) override;
  void SetProfilePref(const std::string& path, base::Value value) override;
  bool FindProfilePref(const std::string& path) const override;
  std::optional<base::Value> GetProfilePref(const std::string& path) override;
  void ClearProfilePref(const std::string& path) override;
  bool HasProfilePrefPath(const std::string& path) const override;
  void SetLocalStatePref(const std::string& path, base::Value value) override;
  bool FindLocalStatePref(const std::string& path) const override;
  std::optional<base::Value> GetLocalStatePref(
      const std::string& path) override;
  void ClearLocalStatePref(const std::string& path) override;
  bool HasLocalStatePrefPath(const std::string& path) const override;
};

#endif  // BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_IOS_H_
