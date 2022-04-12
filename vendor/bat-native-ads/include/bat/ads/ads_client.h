/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_H_

#include <cstdint>
#include <string>
#include <vector>

#include "bat/ads/ads_client_aliases.h"
#include "bat/ads/export.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"

namespace ads {

struct AdNotificationInfo;

class ADS_EXPORT AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Return true if there is an available network connection otherwise return
  // false
  virtual bool IsNetworkConnectionAvailable() const = 0;

  // Return true if the browser is active in the foreground otherwise return
  // false
  virtual bool IsForeground() const = 0;

  // Return true if the browser is full screen otherwise return false
  virtual bool IsFullScreen() const = 0;

  // Return true if notifications should be displayed otherwise return false
  virtual bool ShouldShowNotifications() = 0;

  // Return true if notifications can be displayed while the browser is inactive
  // otherwise return false
  virtual bool CanShowBackgroundNotifications() const = 0;

  // Show notification
  virtual void ShowNotification(const AdNotificationInfo& ad_notification) = 0;

  // Close notification
  virtual void CloseNotification(const std::string& uuid) = 0;

  // Record an ad event for the specified |id|, |ad_type|, |confirmation_type|
  // and specified |timestamp|
  virtual void RecordAdEventForId(const std::string& id,
                                  const std::string& ad_type,
                                  const std::string& confirmation_type,
                                  const double timestamp) const = 0;

  // Get a list of ad events for the specified |ad_type| and |confirmation_type|
  virtual std::vector<double> GetAdEvents(
      const std::string& ad_type,
      const std::string& confirmation_type) const = 0;

  // Reset list of ad events for the specified |id|
  virtual void ResetAdEventsForId(const std::string& id) const = 0;

  // Get |max_count| browsing history results for past |days_ago| days from
  // |HistoryService| and return as list of strings
  virtual void GetBrowsingHistory(const int max_count,
                                  const int days_ago,
                                  GetBrowsingHistoryCallback callback) = 0;

  // Fetch and return data. Loading should be performed asynchronously, so that
  // the app remains responsive and should handle incoming data or errors as
  // they arrive. The callback takes 1 argument - |URLResponse| should contain
  // the url, status code, HTTP body and HTTP headers
  virtual void UrlRequest(mojom::UrlRequestPtr url_request,
                          UrlRequestCallback callback) = 0;

  // Save a value to persistent storage. The callback takes one argument -
  // |bool| should be set to |true| if successful otherwise should be set to
  // |false|
  virtual void Save(const std::string& name,
                    const std::string& value,
                    ResultCallback callback) = 0;

  // Load a value from persistent storage. The callback takes 2 arguments -
  // |bool| should be set to |true| if successful otherwise should be set to
  // |false|. |value| should contain the persisted value
  virtual void Load(const std::string& name, LoadCallback callback) = 0;

  // Load ads resource for name and version from persistent storage.
  virtual void LoadAdsResource(const std::string& name,
                               const int version,
                               LoadCallback callback) = 0;

  // Load ads resource as base::File for name and version from persistent
  // storage.
  virtual void LoadAdsFileResource(const std::string& name,
                                   const int version,
                                   LoadFileCallback callback) = 0;

  // Should return the resource for given |id|
  virtual std::string LoadResourceForId(const std::string& id) = 0;

  // Clears the currently scheduled captcha, if any
  virtual void ClearScheduledCaptcha() = 0;

  // Retrieves the captcha scheduled for the given |payment_id|, if
  // any. If there is a scheduled captcha that the user must solve in
  // order to proceed, |callback| will return the captcha id;
  // otherwise, |callback| will return the empty string.
  virtual void GetScheduledCaptcha(const std::string& payment_id,
                                   GetScheduledCaptchaCallback callback) = 0;

  // Show a notification indicating that a scheduled captcha with the given
  // |captcha_id| must be solved to resume Ads for the given |payment_id|
  virtual void ShowScheduledCaptchaNotification(
      const std::string& payment_id,
      const std::string& captcha_id) = 0;

  // Run database transaction. The callback takes one argument -
  // |mojom::DBCommandResponsePtr|
  virtual void RunDBTransaction(mojom::DBTransactionPtr transaction,
                                RunDBTransactionCallback callback) = 0;

  // Should be called when ad rewards have changed, i.e. to refresh the UI
  virtual void OnAdRewardsChanged() = 0;

  // Record P2A event
  virtual void RecordP2AEvent(const std::string& name,
                              const mojom::P2AEventType type,
                              const std::string& value) = 0;

  // Add federated log
  virtual void LogTrainingCovariates(
      const brave_federated::mojom::TrainingCovariatesPtr
          training_covariates) = 0;

  // Log diagnostic information
  virtual void Log(const char* file,
                   const int line,
                   const int verbose_level,
                   const std::string& message) = 0;

  // Preferences
  virtual bool GetBooleanPref(const std::string& path) const = 0;

  virtual void SetBooleanPref(const std::string& path, const bool value) = 0;

  virtual int GetIntegerPref(const std::string& path) const = 0;

  virtual void SetIntegerPref(const std::string& path, const int value) = 0;

  virtual double GetDoublePref(const std::string& path) const = 0;

  virtual void SetDoublePref(const std::string& path, const double value) = 0;

  virtual std::string GetStringPref(const std::string& path) const = 0;

  virtual void SetStringPref(const std::string& path,
                             const std::string& value) = 0;

  virtual int64_t GetInt64Pref(const std::string& path) const = 0;

  virtual void SetInt64Pref(const std::string& path, const int64_t value) = 0;

  virtual uint64_t GetUint64Pref(const std::string& path) const = 0;

  virtual void SetUint64Pref(const std::string& path, const uint64_t value) = 0;

  virtual void ClearPref(const std::string& path) = 0;

  virtual bool HasPrefPath(const std::string& path) const = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_H_
