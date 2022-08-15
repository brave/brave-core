/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/ads_client.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"

namespace base {
class Time;
}  // namespace base

namespace ads {
struct NotificationAdInfo;
}  // namespace ads

namespace bat_ads {

class BatAdsClientMojoBridge
    : public ads::AdsClient {
 public:
  explicit BatAdsClientMojoBridge(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info);

  ~BatAdsClientMojoBridge() override;

  BatAdsClientMojoBridge(const BatAdsClientMojoBridge&) = delete;
  BatAdsClientMojoBridge& operator=(const BatAdsClientMojoBridge&) = delete;

  // AdsClient:
  bool IsNetworkConnectionAvailable() const override;

  bool IsBrowserActive() const override;
  bool IsBrowserInFullScreenMode() const override;

  bool CanShowNotificationAds() override;
  bool CanShowNotificationAdsWhileBrowserIsBackgrounded() const override;
  void ShowNotificationAd(const ads::NotificationAdInfo& ad) override;
  void CloseNotificationAd(const std::string& placement_id) override;

  void UpdateAdRewards() override;

  void RecordAdEventForId(const std::string& id,
                          const std::string& ad_type,
                          const std::string& confirmation_type,
                          const base::Time time) const override;
  std::vector<base::Time> GetAdEventHistory(
      const std::string& ad_type,
      const std::string& confirmation_type) const override;
  void ResetAdEventHistoryForId(const std::string& id) const override;

  void GetBrowsingHistory(const int max_count,
                          const int days_ago,
                          ads::GetBrowsingHistoryCallback callback) override;

  void UrlRequest(ads::mojom::UrlRequestInfoPtr url_request,
                  ads::UrlRequestCallback callback) override;

  void Save(const std::string& name,
            const std::string& value,
            ads::SaveCallback callback) override;
  void Load(
      const std::string& name,
      ads::LoadCallback callback) override;
  void LoadFileResource(const std::string& id,
                        const int version,
                        ads::LoadFileCallback callback) override;
  std::string LoadDataResource(const std::string& name) override;

  void GetScheduledCaptcha(const std::string& payment_id,
                           ads::GetScheduledCaptchaCallback callback) override;
  void ShowScheduledCaptchaNotification(const std::string& payment_id,
                                        const std::string& captcha_id) override;
  void ClearScheduledCaptcha() override;

  void RunDBTransaction(ads::mojom::DBTransactionInfoPtr transaction,
                        ads::RunDBTransactionCallback callback) override;

  void RecordP2AEvent(const std::string& name,
                      base::Value::List value) override;

  void LogTrainingInstance(std::vector<brave_federated::mojom::CovariateInfoPtr>
                               training_instance) override;

  bool GetBooleanPref(const std::string& path) const override;
  void SetBooleanPref(const std::string& path, const bool value) override;
  int GetIntegerPref(const std::string& path) const override;
  void SetIntegerPref(const std::string& path, const int value) override;
  double GetDoublePref(const std::string& path) const override;
  void SetDoublePref(const std::string& path, const double value) override;
  std::string GetStringPref(const std::string& path) const override;
  void SetStringPref(const std::string& path,
                     const std::string& value) override;
  int64_t GetInt64Pref(const std::string& path) const override;
  void SetInt64Pref(const std::string& path, const int64_t value) override;
  uint64_t GetUint64Pref(const std::string& path) const override;
  void SetUint64Pref(
      const std::string& path,
      const uint64_t value) override;
  base::Time GetTimePref(const std::string& path) const override;
  void SetTimePref(const std::string& path, const base::Time value) override;
  absl::optional<base::Value::Dict> GetDictPref(
      const std::string& path) const override;
  void SetDictPref(const std::string& path, base::Value::Dict value) override;
  absl::optional<base::Value::List> GetListPref(
      const std::string& path) const override;
  void SetListPref(const std::string& path, base::Value::List value) override;
  void ClearPref(const std::string& path) override;
  bool HasPrefPath(const std::string& path) const override;

  void Log(const char* file,
           const int line,
           const int verbose_level,
           const std::string& message) override;

 private:
  bool connected() const;

  mojo::AssociatedRemote<mojom::BatAdsClient> bat_ads_client_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
