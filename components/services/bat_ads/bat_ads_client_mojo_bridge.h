/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ads/ads_client.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"

namespace bat_ads {

class BatAdsClientMojoBridge : public ads::AdsClient {
 public:
  explicit BatAdsClientMojoBridge(
      mojom::BatAdsClientAssociatedPtrInfo client_info);
  ~BatAdsClientMojoBridge() override;

  // AdsClient implementation
  bool IsAdsEnabled() const override;
  bool IsForeground() const override;
  const std::string GetAdsLocale() const override;
  uint64_t GetAdsPerHour() const override;
  uint64_t GetAdsPerDay() const override;
  void GetClientInfo(ads::ClientInfo* info) const override;
  const std::vector<std::string> GetLocales() const override;
  const std::string GenerateUUID() const override;
  void ShowNotification(std::unique_ptr<ads::NotificationInfo> info) override;
  void SetCatalogIssuers(std::unique_ptr<ads::IssuersInfo> info) override;
  void ConfirmAd(std::unique_ptr<ads::NotificationInfo> info) override;
  uint32_t SetTimer(const uint64_t time_offset) override;
  void KillTimer(uint32_t timer_id) override;
  void URLRequest(const std::string& url,
                  const std::vector<std::string>& headers,
                  const std::string& content,
                  const std::string& content_type,
                  ads::URLRequestMethod method,
                  ads::URLRequestCallback callback) override;
  void Save(const std::string& name,
            const std::string& value,
            ads::OnSaveCallback callback) override;
  void Load(const std::string& name,
            ads::OnLoadCallback callback) override;
  void SaveBundleState(
      std::unique_ptr<ads::BundleState> bundle_state,
      ads::OnSaveCallback callback) override;
  const std::string LoadJsonSchema(const std::string& name) override;
  void Reset(const std::string& name,
             ads::OnResetCallback callback) override;
  void GetAds(
      const std::string& category,
      ads::OnGetAdsCallback callback) override;
  void LoadSampleBundle(ads::OnLoadSampleBundleCallback callback) override;
  void EventLog(const std::string& json) override;
  std::unique_ptr<ads::LogStream> Log(
      const char* file,
      int line,
      const ads::LogLevel log_level) const override;
  void SetIdleThreshold(const int threshold) override;
  bool IsNotificationsAvailable() const override;
  void LoadUserModelForLocale(
      const std::string& locale,
      ads::OnLoadCallback callback) const override;
  bool IsNetworkConnectionAvailable() override;

 private:
  bool connected() const;

  mojom::BatAdsClientAssociatedPtr bat_ads_client_;

  DISALLOW_COPY_AND_ASSIGN(BatAdsClientMojoBridge);
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
