/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/export.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

namespace ads {

using ResultCallback = std::function<void(const Result)>;

using LoadCallback = std::function<void(const Result, const std::string&)>;

using UrlRequestCallback = std::function<void(const UrlResponse&)>;

using RunDBTransactionCallback = std::function<void(DBCommandResponsePtr)>;

using GetBrowsingHistoryCallback =
    std::function<void(const std::vector<std::string>&)>;

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

  // Record an ad event for the specified |ad_type|, |confirmation_type| and
  // specified |timestamp|
  virtual void RecordAdEvent(const std::string& ad_type,
                             const std::string& confirmation_type,
                             const uint64_t timestamp) const = 0;

  // Get a list of ad events for the specified |ad_type| and |confirmation_type|
  virtual std::vector<uint64_t> GetAdEvents(
      const std::string& ad_type,
      const std::string& confirmation_type) const = 0;

  // Fetch and return data. Loading should be performed asynchronously, so that
  // the app remains responsive and should handle incoming data or errors as
  // they arrive. The callback takes 1 argument - |URLResponse| should contain
  // the url, status code, HTTP body and HTTP headers
  virtual void UrlRequest(UrlRequestPtr url_request,
                          UrlRequestCallback callback) = 0;

  // Save a value to persistent storage. The callback takes one argument -
  // |Result| should be set to |SUCCESS| if successful otherwise should be set
  // to |FAILED|
  virtual void Save(const std::string& name,
                    const std::string& value,
                    ResultCallback callback) = 0;

  // Load a value from persistent storage. The callback takes 2 arguments -
  // |Result| should be set to |SUCCESS| if successful otherwise should be set
  // to |FAILED|. |value| should contain the persisted value
  virtual void Load(const std::string& name, LoadCallback callback) = 0;

  // Load ads resource for name and version from persistent storage.
  virtual void LoadAdsResource(const std::string& name,
                               const int version,
                               LoadCallback callback) = 0;

  // Get |max_count| browsing history results for past |days_ago| days from
  // |HistoryService| and return as list of strings
  virtual void GetBrowsingHistory(const int max_count,
                                  const int days_ago,
                                  GetBrowsingHistoryCallback callback) = 0;

  // Should return the resource for given |id|
  virtual std::string LoadResourceForId(const std::string& id) = 0;

  // Run database transaction. The callback takes one argument -
  // |DBCommandResponsePtr|
  virtual void RunDBTransaction(DBTransactionPtr transaction,
                                RunDBTransactionCallback callback) = 0;

  // Should be called when ad rewards have changed, i.e. to refresh the UI
  virtual void OnAdRewardsChanged() = 0;

  // Record P2A event
  virtual void RecordP2AEvent(const std::string& name,
                              const ads::P2AEventType type,
                              const std::string& value) = 0;

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
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_H_
