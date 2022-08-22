/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_IOS_H_
#define BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_IOS_H_

#import <Foundation/Foundation.h>
#include <string>
#include <vector>
#import "bat/ads/ads_client.h"

@protocol AdsClientBridge;

class AdsClientIOS : public ads::AdsClient {
 public:
  explicit AdsClientIOS(id<AdsClientBridge> bridge);
  ~AdsClientIOS() override;

 private:
  __unsafe_unretained id<AdsClientBridge> bridge_;

  bool IsNetworkConnectionAvailable() const override;
  bool IsBrowserActive() const override;
  bool IsBrowserInFullScreenMode() const override;
  bool CanShowNotificationAdsWhileBrowserIsBackgrounded() const override;
  void ShowNotificationAd(const ads::NotificationAdInfo& ad) override;
  bool CanShowNotificationAds() override;
  void CloseNotificationAd(const std::string& placement_id) override;
  void RecordAdEventForId(const std::string& id,
                          const std::string& ad_type,
                          const std::string& confirmation_type,
                          const base::Time time) const override;
  std::vector<base::Time> GetAdEventHistory(
      const std::string& ad_type,
      const std::string& confirmation_type) const override;
  void ResetAdEventHistoryForId(const std::string& id) const override;
  void UrlRequest(ads::mojom::UrlRequestInfoPtr url_request,
                  ads::UrlRequestCallback callback) override;
  void Save(const std::string& name,
            const std::string& value,
            ads::SaveCallback callback) override;
  void Load(const std::string& name, ads::LoadCallback callback) override;
  void LoadFileResource(const std::string& id,
                        const int version,
                        ads::LoadFileCallback callback) override;
  void GetBrowsingHistory(const int max_count,
                          const int days_ago,
                          ads::GetBrowsingHistoryCallback callback) override;
  std::string LoadDataResource(const std::string& name) override;
  void ClearScheduledCaptcha() override;
  void GetScheduledCaptcha(const std::string& payment_id,
                           ads::GetScheduledCaptchaCallback callback) override;
  void ShowScheduledCaptchaNotification(const std::string& payment_id,
                                        const std::string& captcha_id) override;
  void Log(const char* file,
           const int line,
           const int verbose_level,
           const std::string& message) override;
  void RunDBTransaction(ads::mojom::DBTransactionInfoPtr transaction,
                        ads::RunDBTransactionCallback callback) override;
  void UpdateAdRewards() override;
  void SetBooleanPref(const std::string& path, const bool value) override;
  bool GetBooleanPref(const std::string& path) const override;
  void SetIntegerPref(const std::string& path, const int value) override;
  int GetIntegerPref(const std::string& path) const override;
  void SetDoublePref(const std::string& path, const double value) override;
  double GetDoublePref(const std::string& path) const override;
  void SetStringPref(const std::string& path,
                     const std::string& value) override;
  std::string GetStringPref(const std::string& path) const override;
  void SetInt64Pref(const std::string& path, const int64_t value) override;
  int64_t GetInt64Pref(const std::string& path) const override;
  void SetUint64Pref(const std::string& path, const uint64_t value) override;
  uint64_t GetUint64Pref(const std::string& path) const override;
  void SetTimePref(const std::string& path, const base::Time value) override;
  base::Time GetTimePref(const std::string& path) const override;
  void SetDictPref(const std::string& path, base::Value::Dict value) override;
  absl::optional<base::Value::Dict> GetDictPref(
      const std::string& path) const override;
  void SetListPref(const std::string& path, base::Value::List value) override;
  absl::optional<base::Value::List> GetListPref(
      const std::string& path) const override;
  void ClearPref(const std::string& path) override;
  bool HasPrefPath(const std::string& path) const override;
  void RecordP2AEvent(const std::string& name,
                      base::Value::List value) override;
  void LogTrainingInstance(
      const std::vector<brave_federated::mojom::CovariateInfoPtr>
          training_instance) override;
};

#endif  // BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_IOS_H_
