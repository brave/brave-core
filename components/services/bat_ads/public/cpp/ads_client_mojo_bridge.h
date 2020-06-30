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
  bool GetUserModelLanguages(
      std::vector<std::string>* out_languages) override;
  void GetUserModelLanguages(
      GetUserModelLanguagesCallback callback) override;
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
  bool LoadJsonSchema(
      const std::string& name,
      std::string* out_json) override;
  void LoadJsonSchema(
      const std::string& name,
      LoadJsonSchemaCallback callback) override;
  bool GetClientInfo(
      const std::string& client_info,
      std::string* out_client_info) override;
  void GetClientInfo(
      const std::string& client_info,
      GetClientInfoCallback callback) override;
  void Log(
      const std::string& file,
      const int32_t line,
      const int32_t verbose_level,
      const std::string& message) override;
  void SetIdleThreshold(
      const int32_t threshold) override;
  void Load(
      const std::string& name,
      LoadCallback callback) override;
  void Save(
      const std::string& name,
      const std::string& value,
      SaveCallback callback) override;
  void Reset(
      const std::string& name,
      ResetCallback callback) override;
  void LoadUserModelForLanguage(
      const std::string& locale,
      LoadUserModelForLanguageCallback callback) override;
  void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const int32_t method,
      URLRequestCallback callback) override;
  void ShowNotification(
      const std::string& notification_info) override;
  void CloseNotification(
      const std::string& uuid) override;
  void SetCatalogIssuers(
      const std::string& issuers_info) override;
  void ConfirmAd(
      const std::string& json,
      const std::string& confirmation_type) override;
  void ConfirmAction(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const std::string& confirmation_type) override;
  void RunDBTransaction(
      ads::DBTransactionPtr transaction,
      RunDBTransactionCallback callback) override;

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

  static void OnLoad(
      CallbackHolder<LoadCallback>* holder,
      const ads::Result result,
      const std::string& value);

  static void OnSave(
      CallbackHolder<SaveCallback>* holder,
      const ads::Result result);

  static void OnReset(
      CallbackHolder<ResetCallback>* holder,
      const ads::Result result);

  static void OnLoadUserModelForLanguage(
      CallbackHolder<LoadUserModelForLanguageCallback>* holder,
      const ads::Result result,
      const std::string& value);

  static void OnURLRequest(
      CallbackHolder<URLRequestCallback>* holder,
      const int response_status_code,
      const std::string& content,
      const std::map<std::string, std::string>& headers);

  static void OnRunDBTransaction(
      CallbackHolder<RunDBTransactionCallback>* holder,
      ads::DBCommandResponsePtr response);

  ads::AdsClient* ads_client_;  // NOT OWNED
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_  // NOLINT
