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

  // Should return |true| if ads is enabled; otherwise, should return |false|
  virtual bool IsEnabled() const = 0;

  // Should return |true| if allow ad conversion tracking is enabled; otherwise,
  // should return |false|
  virtual bool ShouldAllowAdConversionTracking() const = 0;

  // Should return the maximum number of ads that can be shown per hour
  virtual uint64_t GetAdsPerHour() const = 0;

  // Should return the maximum number of ads that can be shown per day
  virtual uint64_t GetAdsPerDay() const = 0;

  // Should return |true| if ads subdivision targeting is allowed; otherwise,
  // should return |false|
  virtual bool ShouldAllowAdsSubdivisionTargeting() const = 0;

  // Set if ads subdivision targeting is allowed
  virtual void SetAllowAdsSubdivisionTargeting(
      const bool should_allow) = 0;

  // Should return the ads subdivision targeting code
  virtual std::string GetAdsSubdivisionTargetingCode() const = 0;

  // Set the ads subdivision targeting code
  virtual void SetAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) = 0;

  // Should return the automatically detected ads subdivision targeting code
  virtual std::string
      GetAutomaticallyDetectedAdsSubdivisionTargetingCode() const = 0;

  // Set the automatically detected ads subdivision targeting code
  virtual void SetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) = 0;

  // Set the idle threshold specified in seconds for how long a user should be
  // idle before |OnUnIdle| is called. This call is optional for mobile devices
  virtual void SetIdleThreshold(
      const int threshold) = 0;

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
      const std::unique_ptr<AdNotificationInfo> info) = 0;

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
};

}  // namespace ads

#endif  // BAT_ADS_ADS_CLIENT_H_
