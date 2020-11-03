/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ADS_CLIENT_H_
#define BAT_ADS_ADS_CLIENT_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/export.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

namespace ads {

using ResultCallback = std::function<void(const Result)>;

using LoadCallback = std::function<void(const Result, const std::string&)>;

using UrlRequestCallback = std::function<void(const UrlResponse&)>;

using RunDBTransactionCallback = std::function<void(DBCommandResponsePtr)>;

class ADS_EXPORT AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Should return |true| if there is an available network connection;
  // otherwise, should return |false|
  virtual bool IsNetworkConnectionAvailable() const = 0;

  // Should return |true| if the browser is active in the foreground; otherwise,
  // should return |false|
  virtual bool IsForeground() const = 0;

  // Should return true if background notifications are allowed
  virtual bool CanShowBackgroundNotifications() const = 0;

  // Should show a notification
  virtual void ShowNotification(
      const AdNotificationInfo& ad_notification) = 0;

  // Should return |true| if notifications can be displayed; otherwise should
  // return |false|
  virtual bool ShouldShowNotifications() = 0;

  // Should close a notification
  virtual void CloseNotification(
      const std::string& uuid) = 0;

  // Should fetch and return data. Loading should be performed asynchronously,
  // so that the app remains responsive and should handle incoming data or
  // errors as they arrive. The callback takes 1 argument — |URLResponse| should
  // contain the url, status code, HTTP body and HTTP headers
  virtual void UrlRequest(
      UrlRequestPtr url_request,
      UrlRequestCallback callback) = 0;

  // Should save a value to persistent storage. The callback takes one argument
  // — |Result| should be set to |SUCCESS| if successful; otherwise, should be
  // set to |FAILED|
  virtual void Save(
      const std::string& name,
      const std::string& value,
      ResultCallback callback) = 0;

  // Should load user model for id from persistent storage. The callback takes 2
  // arguments — |Result| should be set to |SUCCESS| if successful; otherwise,
  // should be set to |FAILED|. |value| should contain the user model
  virtual void LoadUserModelForId(
      const std::string& name, LoadCallback callback) = 0;

  // Should record a P2A event of the given type
  virtual void RecordP2AEvent(
      const std::string& name,
      const ads::P2AEventType type,
      const std::string& value) = 0;

  // Should load a value from persistent storage. The callback takes 2 arguments
  // — |Result| should be set to |SUCCESS| if successful; otherwise, should be
  // set to |FAILED|. |value| should contain the persisted value
  virtual void Load(
      const std::string& name, LoadCallback callback) = 0;

  // Should load a resource from persistent storage
  virtual std::string LoadResourceForId(
      const std::string& id) = 0;

  // Should run a database transaction. The callback takes one argument -
  // |DBCommandResponsePtr|
  virtual void RunDBTransaction(
      DBTransactionPtr transaction,
      RunDBTransactionCallback callback) = 0;

  // Should be called when ad rewards has changed
  virtual void OnAdRewardsChanged() = 0;

  // Verbose level logging
  virtual void Log(
      const char* file,
      const int line,
      const int verbose_level,
      const std::string& message) = 0;

  // Preferences
  virtual bool GetBooleanPref(
      const std::string& path) const = 0;

  virtual void SetBooleanPref(
      const std::string& path,
      const bool value) = 0;

  virtual int GetIntegerPref(
      const std::string& path) const = 0;

  virtual void SetIntegerPref(
      const std::string& path,
      const int value) = 0;

  virtual double GetDoublePref(
      const std::string& path) const = 0;

  virtual void SetDoublePref(
      const std::string& path,
      const double value) = 0;

  virtual std::string GetStringPref(
      const std::string& path) const = 0;

  virtual void SetStringPref(
      const std::string& path,
      const std::string& value) = 0;

  virtual int64_t GetInt64Pref(
      const std::string& path) const = 0;

  virtual void SetInt64Pref(
      const std::string& path,
      const int64_t value) = 0;

  virtual uint64_t GetUint64Pref(
      const std::string& path) const = 0;

  virtual void SetUint64Pref(
      const std::string& path,
      const uint64_t value) = 0;

  virtual void ClearPref(
      const std::string& path) = 0;
};

}  // namespace ads

#endif  // BAT_ADS_ADS_CLIENT_H_
