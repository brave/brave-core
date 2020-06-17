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
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"

namespace bat_ads {

class BatAdsClientMojoBridge
    : public ads::AdsClient {
 public:
  explicit BatAdsClientMojoBridge(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info);

  ~BatAdsClientMojoBridge() override;

  BatAdsClientMojoBridge(const BatAdsClientMojoBridge&) = delete;
  BatAdsClientMojoBridge& operator=(const BatAdsClientMojoBridge&) = delete;

  // AdsClient implementation
  bool IsEnabled() const override;

  bool ShouldAllowAdConversionTracking() const override;

  bool CanShowBackgroundNotifications() const override;

  uint64_t GetAdsPerHour() const override;
  uint64_t GetAdsPerDay() const override;

  bool ShouldAllowAdsSubdivisionTargeting() const override;
  void SetAllowAdsSubdivisionTargeting(
      const bool should_allow) override;

  std::string GetAdsSubdivisionTargetingCode() const override;
  void SetAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) override;

  std::string
  GetAutomaticallyDetectedAdsSubdivisionTargetingCode() const override;
  void SetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) override;

  void GetClientInfo(
      ads::ClientInfo* info) const override;

  bool IsNetworkConnectionAvailable() const override;

  void SetIdleThreshold(
      const int threshold) override;

  bool IsForeground() const override;

  std::vector<std::string> GetUserModelLanguages() const override;
  void LoadUserModelForLanguage(
      const std::string& language,
      ads::LoadCallback callback) const override;

  void ShowNotification(
      std::unique_ptr<ads::AdNotificationInfo> info) override;
  bool ShouldShowNotifications() override;
  void CloseNotification(
      const std::string& uuid) override;

  void SetCatalogIssuers(
      std::unique_ptr<ads::IssuersInfo> info) override;

  void ConfirmAd(
      const ads::AdInfo& info,
      const ads::ConfirmationType confirmation_type) override;
  void ConfirmAction(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const ads::ConfirmationType confirmation_type) override;

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
      ads::ResultCallback callback) override;
  void Load(
      const std::string& name,
      ads::LoadCallback callback) override;
  void Reset(
      const std::string& name,
      ads::ResultCallback callback) override;

  std::string LoadJsonSchema(
      const std::string& name) override;

  void RunDBTransaction(
      ads::DBTransactionPtr transaction,
      ads::RunDBTransactionCallback callback) override;

  void Log(
      const char* file,
      const int line,
      const int verbose_level,
      const std::string& message) override;

 private:
  bool connected() const;

  mojo::AssociatedRemote<mojom::BatAdsClient> bat_ads_client_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
