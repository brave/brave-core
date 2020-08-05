/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "bat/ads/ads_client.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace bat_ads {

class AdsClientMojoBridge
    : public mojom::BatAdsClient,
      public base::SupportsWeakPtr<AdsClientMojoBridge> {
 public:
  explicit AdsClientMojoBridge(
      ads::AdsClient* ads_client);

  ~AdsClientMojoBridge() override;

  AdsClientMojoBridge(const AdsClientMojoBridge&) = delete;
  AdsClientMojoBridge& operator=(const AdsClientMojoBridge&) = delete;

  // Overridden from BatAdsClient:
  bool IsEnabled(
      bool* out_is_enabled) override;
  void IsEnabled(
      IsEnabledCallback callback) override;
  bool ShouldAllowAdConversionTracking(
      bool* out_should_allow) override;
  void ShouldAllowAdConversionTracking(
      ShouldAllowAdConversionTrackingCallback callback) override;
  bool IsForeground(
      bool* out_is_foreground) override;
  void IsForeground(
      IsForegroundCallback callback) override;
  bool GetAdsPerHour(
      uint64_t* out_ads_per_hour) override;
  void GetAdsPerHour(
      GetAdsPerHourCallback callback) override;
  bool GetAdsPerDay(
      uint64_t* out_ads_per_day) override;
  void GetAdsPerDay(
      GetAdsPerDayCallback callback) override;
  bool ShouldAllowAdsSubdivisionTargeting(
      bool* out_should_allow) override;
  void ShouldAllowAdsSubdivisionTargeting(
      ShouldAllowAdsSubdivisionTargetingCallback callback) override;
  void SetAllowAdsSubdivisionTargeting(
      const bool should_allow) override;
  bool GetAdsSubdivisionTargetingCode(
      std::string* out_subdivision_targeting_code) override;
  void GetAdsSubdivisionTargetingCode(
      GetAdsSubdivisionTargetingCodeCallback callback) override;
  void SetAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) override;
  bool GetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      std::string* out_subdivision_targeting_code) override;
  void GetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      GetAutomaticallyDetectedAdsSubdivisionTargetingCodeCallback callback)
          override;
  void SetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) override;
  bool IsNetworkConnectionAvailable(
      bool* out_available) override;
  bool CanShowBackgroundNotifications(
      bool* out_can_show) override;
  void CanShowBackgroundNotifications(
      CanShowBackgroundNotificationsCallback callback) override;
  void IsNetworkConnectionAvailable(
      IsNetworkConnectionAvailableCallback callback) override;
  bool ShouldShowNotifications(
      bool* out_should_show) override;
  void ShouldShowNotifications(
      ShouldShowNotificationsCallback callback) override;
  bool LoadResourceForId(
      const std::string& id,
      std::string* out_value) override;
  void LoadResourceForId(
      const std::string& id,
      LoadResourceForIdCallback callback) override;
  void Log(
      const std::string& file,
      const int32_t line,
      const int32_t verbose_level,
      const std::string& message) override;
  void SetIdleThreshold(
      const int32_t threshold) override;
  void LoadUserModelForId(
      const std::string& id,
      LoadCallback callback) override;
  void Load(
      const std::string& name,
      LoadCallback callback) override;
  void Save(
      const std::string& name,
      const std::string& value,
      SaveCallback callback) override;
  void UrlRequest(
      ads::UrlRequestPtr url_request,
      UrlRequestCallback callback) override;
  void ShowNotification(
      const std::string& notification_info) override;
  void CloseNotification(
      const std::string& uuid) override;
  void RunDBTransaction(
      ads::DBTransactionPtr transaction,
      RunDBTransactionCallback callback) override;
  void OnAdRewardsChanged() override;

 private:
  // workaround to pass base::OnceCallback into std::bind
  template <typename Callback>
  class CallbackHolder {
   public:
    CallbackHolder(
        base::WeakPtr<AdsClientMojoBridge> client,
        Callback callback)
        : client_(client),
          callback_(std::move(callback)) {}

    ~CallbackHolder() = default;

    bool is_valid() {
      return !!client_.get();
    }

    Callback& get() {
      return callback_;
    }

   private:
    base::WeakPtr<AdsClientMojoBridge> client_;
    Callback callback_;
  };

  static void OnLoadUserModelForId(
      CallbackHolder<LoadCallback>* holder,
      const ads::Result result,
      const std::string& value);

  static void OnLoad(
      CallbackHolder<LoadCallback>* holder,
      const ads::Result result,
      const std::string& value);

  static void OnSave(
      CallbackHolder<SaveCallback>* holder,
      const ads::Result result);

  static void OnURLRequest(
      CallbackHolder<UrlRequestCallback>* holder,
      const ads::UrlResponse& response);

  static void OnRunDBTransaction(
      CallbackHolder<RunDBTransactionCallback>* holder,
      ads::DBCommandResponsePtr response);

  ads::AdsClient* ads_client_;  // NOT OWNED
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_
