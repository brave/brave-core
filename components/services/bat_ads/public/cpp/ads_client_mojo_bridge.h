/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_

#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "bat/ads/ads_client.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"

class GURL;

namespace base {
class Time;
}  // namespace base

namespace bat_ads {

class AdsClientMojoBridge
    : public mojom::BatAdsClient,
      public base::SupportsWeakPtr<AdsClientMojoBridge> {
 public:
  explicit AdsClientMojoBridge(ads::AdsClient* ads_client);
  AdsClientMojoBridge(const AdsClientMojoBridge&) = delete;
  AdsClientMojoBridge& operator=(const AdsClientMojoBridge&) = delete;
  ~AdsClientMojoBridge() override;

 private:
  // TODO(https://github.com/brave/brave-browser/issues/20940) Workaround to
  // pass |base::OnceCallback| into |std::bind| until we refactor Brave Ads
  // |std::function| to |base::OnceCallback|.
  template <typename T>
  class CallbackHolder {
   public:
    CallbackHolder(base::WeakPtr<AdsClientMojoBridge> client, T callback)
        : client_(client), callback_(std::move(callback)) {}

    ~CallbackHolder() = default;

    bool is_valid() { return !!client_.get(); }

    T& get() { return callback_; }

   private:
    base::WeakPtr<AdsClientMojoBridge> client_;
    T callback_;
  };

  static void OnURLRequest(CallbackHolder<UrlRequestCallback>* callback_holder,
                           const ads::mojom::UrlResponseInfo& url_response);

  static void OnSave(CallbackHolder<SaveCallback>* callback_holder,
                     const bool success);

  // BatAdsClient:
  bool IsNetworkConnectionAvailable(bool* out_value) override;
  void IsNetworkConnectionAvailable(
      IsNetworkConnectionAvailableCallback callback) override;

  bool IsBrowserActive(bool* out_value) override;
  void IsBrowserActive(IsBrowserActiveCallback callback) override;
  bool IsBrowserInFullScreenMode(bool* out_value) override;
  void IsBrowserInFullScreenMode(
      IsBrowserInFullScreenModeCallback callback) override;

  bool CanShowNotificationAds(bool* out_value) override;
  void CanShowNotificationAds(CanShowNotificationAdsCallback callback) override;
  bool CanShowNotificationAdsWhileBrowserIsBackgrounded(
      bool* out_value) override;
  void CanShowNotificationAdsWhileBrowserIsBackgrounded(
      CanShowNotificationAdsWhileBrowserIsBackgroundedCallback callback)
      override;
  void ShowNotificationAd(base::Value::Dict dict) override;
  void CloseNotificationAd(const std::string& placement_id) override;

  void UpdateAdRewards() override;

  void RecordAdEventForId(const std::string& id,
                          const std::string& ad_type,
                          const std::string& confirmation_type,
                          const base::Time time) override;
  bool GetAdEventHistory(const std::string& ad_type,
                         const std::string& confirmation_type,
                         std::vector<base::Time>* out_value) override;
  void GetAdEventHistory(const std::string& ad_type,
                         const std::string& confirmation_type,
                         GetAdEventHistoryCallback callback) override;
  void ResetAdEventHistoryForId(const std::string& id) override;

  void GetBrowsingHistory(const int max_count,
                          const int days_ago,
                          GetBrowsingHistoryCallback callback) override;

  void UrlRequest(ads::mojom::UrlRequestInfoPtr url_request,
                  UrlRequestCallback callback) override;

  void Save(const std::string& name,
            const std::string& value,
            SaveCallback callback) override;
  void Load(const std::string& name, LoadCallback callback) override;
  void LoadFileResource(const std::string& id,
                        const int version,
                        LoadFileResourceCallback callback) override;
  bool LoadDataResource(const std::string& name,
                        std::string* out_value) override;
  void LoadDataResource(const std::string& name,
                        LoadDataResourceCallback callback) override;

  void GetScheduledCaptcha(const std::string& payment_id,
                           GetScheduledCaptchaCallback callback) override;
  void ShowScheduledCaptchaNotification(const std::string& payment_id,
                                        const std::string& captcha_id) override;
  void ClearScheduledCaptcha() override;

  void RunDBTransaction(ads::mojom::DBTransactionInfoPtr transaction,
                        RunDBTransactionCallback callback) override;

  void RecordP2AEvent(const std::string& name,
                      base::Value::List value) override;

  void LogTrainingInstance(std::vector<brave_federated::mojom::CovariateInfoPtr>
                               training_instance) override;

  void GetBooleanPref(
      const std::string& path,
      GetBooleanPrefCallback callback) override;
  void SetBooleanPref(
      const std::string& path,
      const bool value) override;
  void GetIntegerPref(
      const std::string& path,
      GetIntegerPrefCallback callback) override;
  void SetIntegerPref(
      const std::string& path,
      const int value) override;
  void GetDoublePref(
      const std::string& path,
      GetDoublePrefCallback callback) override;
  void SetDoublePref(
      const std::string& path,
      const double value) override;
  void GetStringPref(
      const std::string& path,
      GetStringPrefCallback callback) override;
  void SetStringPref(
      const std::string& path,
      const std::string& value) override;
  void GetInt64Pref(
      const std::string& path,
      GetInt64PrefCallback callback) override;
  void SetInt64Pref(
      const std::string& path,
      const int64_t value) override;
  void GetUint64Pref(
      const std::string& path,
      GetUint64PrefCallback callback) override;
  void SetUint64Pref(
      const std::string& path,
      const uint64_t value) override;
  void GetTimePref(const std::string& path,
                   GetTimePrefCallback callback) override;
  void SetTimePref(const std::string& path, const base::Time value) override;
  void GetDictPref(const std::string& path,
                   GetDictPrefCallback callback) override;
  void SetDictPref(const std::string& path, base::Value::Dict value) override;
  void GetListPref(const std::string& path,
                   GetListPrefCallback callback) override;
  void SetListPref(const std::string& path, base::Value::List value) override;
  void ClearPref(
      const std::string& path) override;
  void HasPrefPath(const std::string& path,
                   HasPrefPathCallback callback) override;

  void Log(const std::string& file,
           const int32_t line,
           const int32_t verbose_level,
           const std::string& message) override;

  raw_ptr<ads::AdsClient> ads_client_ = nullptr;  // NOT OWNED
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_
