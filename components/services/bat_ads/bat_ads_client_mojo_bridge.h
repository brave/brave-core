/* Copyright (c) 2019 The Brave Authors. All rights 
erved.
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
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"

namespace bat_ads {

class BatAdsClientMojoBridge : public ads::AdsClient {
 public:
  explicit BatAdsClientMojoBridge(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info);

  ~BatAdsClientMojoBridge() override;

  // AdsClient implementation
  bool IsEnabled() const override;

  bool ShouldShowPublisherAdsOnPariticipatingSites() const override;

  bool ShouldAllowAdConversionTracking() const override;

  bool CanShowBackgroundNotifications() const override;
  uint64_t GetAdsPerHour() const override;
  uint64_t GetAdsPerDay() const override;

  void GetClientInfo(
      ads::ClientInfo* info) const override;

  const std::string GetLocale() const override;

  bool IsNetworkConnectionAvailable() const override;

  void SetIdleThreshold(
      const int threshold) override;

  bool IsForeground() const override;

  const std::vector<std::string> GetUserModelLanguages() const override;
  void LoadUserModelForLanguage(
      const std::string& language,
      ads::OnLoadCallback callback) const override;

  void ShowNotification(
      std::unique_ptr<ads::AdNotificationInfo> info) override;
  bool ShouldShowNotifications() override;
  void CloseNotification(
      const std::string& id) override;

  void SetCatalogIssuers(
      std::unique_ptr<ads::IssuersInfo> info) override;

  void ConfirmAdNotification(
      std::unique_ptr<ads::AdNotificationInfo> info) override;
  void ConfirmPublisherAd(
      const ads::PublisherAdInfo& info) override;
  void ConfirmAction(
      const std::string& uuid,
      const std::string& creative_set_id,
      const ads::ConfirmationType& type) override;

  uint32_t SetTimer(
      const uint64_t time_offset) override;
  void KillTimer(
      const uint32_t timer_id) override;

  void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ads::URLRequestMethod method,
      ads::URLRequestCallback callback) override;

  void Save(
      const std::string& name,
      const std::string& value,
      ads::OnSaveCallback callback) override;
  void Load(
      const std::string& name,
      ads::OnLoadCallback callback) override;
  void Reset(
      const std::string& name,
      ads::OnResetCallback callback) override;

  const std::string LoadJsonSchema(
      const std::string& name) override;

  void LoadSampleBundle(
      ads::OnLoadSampleBundleCallback callback) override;

  void SaveBundleState(
      std::unique_ptr<ads::BundleState> bundle_state,
      ads::OnSaveCallback callback) override;

  void GetCreativeAdNotifications(
      const std::vector<std::string>& categories,
      ads::OnGetCreativeAdNotificationsCallback callback) override;

  void GetCreativePublisherAds(
      const std::string& url,
      const std::vector<std::string>& categories,
      const std::vector<std::string>& sizes,
      const ads::OnGetCreativePublisherAdsCallback callback) override;

  void GetAdConversions(
      const std::string& url,
      ads::OnGetAdConversionsCallback callback) override;

  void EventLog(
      const std::string& json) const override;

  std::unique_ptr<ads::LogStream> Log(
      const char* file,
      const int line,
      const ads::LogLevel log_level) const override;

 private:
  bool connected() const;

  mojo::AssociatedRemote<mojom::BatAdsClient> bat_ads_client_;

  DISALLOW_COPY_AND_ASSIGN(BatAdsClientMojoBridge);
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
