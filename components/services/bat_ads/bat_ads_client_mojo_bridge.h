/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_

#include <cstdint>
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
  bool CanShowBackgroundNotifications() const override;

  bool IsNetworkConnectionAvailable() const override;

  bool IsForeground() const override;

  bool IsFullScreen() const override;

  void ShowNotification(
      const ads::AdNotificationInfo& ad_notification) override;
  bool ShouldShowNotifications() override;
  void CloseNotification(const std::string& uuid) override;

  void RecordAdEvent(const std::string& ad_type,
                     const std::string& confirmation_type,
                     const uint64_t timestamp) const override;
  std::vector<uint64_t> GetAdEvents(
      const std::string& ad_type,
      const std::string& confirmation_type) const override;

  void UrlRequest(
      ads::UrlRequestPtr url_request,
      ads::UrlRequestCallback callback) override;

  void Save(
      const std::string& name,
      const std::string& value,
      ads::ResultCallback callback) override;
  void LoadAdsResource(const std::string& id,
                       const int version,
                       ads::LoadCallback callback) override;

  void GetBrowsingHistory(const int max_count,
                          const int days_ago,
                          ads::GetBrowsingHistoryCallback callback) override;

  void RecordP2AEvent(
      const std::string& name,
      const ads::P2AEventType type,
      const std::string& value) override;

  void Load(
      const std::string& name,
      ads::LoadCallback callback) override;

  std::string LoadResourceForId(
      const std::string& id) override;

  void RunDBTransaction(
      ads::DBTransactionPtr transaction,
      ads::RunDBTransactionCallback callback) override;

  void OnAdRewardsChanged() override;

  void Log(
      const char* file,
      const int line,
      const int verbose_level,
      const std::string& message) override;

  bool GetBooleanPref(
      const std::string& path) const override;

  void SetBooleanPref(
      const std::string& path,
      const bool value) override;

  int GetIntegerPref(
      const std::string& path) const override;

  void SetIntegerPref(
      const std::string& path,
      const int value) override;

  double GetDoublePref(
      const std::string& path) const override;

  void SetDoublePref(
      const std::string& path,
      const double value) override;

  std::string GetStringPref(
      const std::string& path) const override;

  void SetStringPref(
      const std::string& path,
      const std::string& value) override;

  int64_t GetInt64Pref(
      const std::string& path) const override;

  void SetInt64Pref(
      const std::string& path,
      const int64_t value) override;

  uint64_t GetUint64Pref(
      const std::string& path) const override;

  void SetUint64Pref(
      const std::string& path,
      const uint64_t value) override;

  void ClearPref(
      const std::string& path) override;

 private:
  bool connected() const;

  mojo::AssociatedRemote<mojom::BatAdsClient> bat_ads_client_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
