/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <memory>
#include <functional>

#include "bat/ads/ad_info.h"
#include "bat/ads/bundle_state.h"
#include "bat/ads/client_info.h"
#include "bat/ads/export.h"
#include "bat/ads/notification_info.h"
#include "bat/ads/url_components.h"

namespace ads {

enum ADS_EXPORT LogLevel {
  ERROR = 1,
  WARNING,
  INFO
};

enum ADS_EXPORT URLRequestMethod {
  GET = 0,
  PUT = 1,
  POST = 2
};

enum ADS_EXPORT Result {
  SUCCESS,
  FAILED
};

class ADS_EXPORT LogStream {
 public:
  virtual ~LogStream() = default;
  virtual std::ostream& stream() = 0;
};

using OnSaveCallback = std::function<void(Result)>;
using OnLoadCallback = std::function<void(Result, const std::string&)>;

using OnResetCallback = std::function<void(Result)>;

using OnGetAdsCallback = std::function<void(Result,
  const std::string&, const std::string&, const std::vector<AdInfo>&)>;

using OnLoadSampleBundleCallback = std::function<void(Result,
  const std::string&)>;

using URLRequestCallback = std::function<void(const int, const std::string&,
  const std::map<std::string, std::string>& headers)>;

class ADS_EXPORT AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Gets the status of ads whether enabled or disabled
  virtual bool IsAdsEnabled() const = 0;

  // Gets the locale for ads
  virtual const std::string GetAdsLocale() const = 0;

  // Gets maximum amount of ads that can be shown per hour
  virtual uint64_t GetAdsPerHour() const = 0;

  // Gets maximum amount of ads that can be shown per day
  virtual uint64_t GetAdsPerDay() const = 0;

  // Sets the idle threshold
  virtual void SetIdleThreshold(const int threshold) = 0;

  // Gets network connection is availability
  virtual bool IsNetworkConnectionAvailable() = 0;

  // Gets information about the client
  virtual void GetClientInfo(ClientInfo* info) const = 0;

  // Gets available locales
  virtual const std::vector<std::string> GetLocales() const = 0;

  // Load User Model for specified locale
  virtual void LoadUserModelForLocale(
      const std::string& locale,
      OnLoadCallback callback) const = 0;

  // Generate a v4 UUID
  virtual const std::string GenerateUUID() const = 0;

  // Gets the network SSID or an empty string if not available
  virtual const std::string GetSSID() const = 0;

  // Gets whether notifications are available within the Operating System
  virtual bool IsNotificationsAvailable() const = 0;

  // Shows the notification
  virtual void ShowNotification(
      std::unique_ptr<NotificationInfo> info) = 0;

  // Creates a timer with the specified id and time offset
  virtual uint32_t SetTimer(const uint64_t& time_offset) = 0;

  // Destroys the specified timer
  virtual void KillTimer(uint32_t timer_id) = 0;

  // Starts a URL session task
  virtual void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      URLRequestMethod method,
      URLRequestCallback callback) = 0;

  // Saves a value
  virtual void Save(
      const std::string& name,
      const std::string& value,
      OnSaveCallback callback) = 0;

  // Saves the bundle state
  virtual void SaveBundleState(
    std::unique_ptr<BundleState> state,
    OnSaveCallback callback) = 0;

  // Loads a value
  virtual void Load(const std::string& name, OnLoadCallback callback) = 0;

  // Loads a JSON schema
  virtual const std::string LoadJsonSchema(const std::string& name) = 0;

  // Loads the sample bundle
  virtual void LoadSampleBundle(OnLoadSampleBundleCallback callback) = 0;

  // Reset a previously saved value
  virtual void Reset(
      const std::string& name,
      OnResetCallback callback) = 0;

  // Gets ads for specified region and category
  virtual void GetAds(
      const std::string& region,
      const std::string& category,
      OnGetAdsCallback callback) = 0;

  // Gets the components of a URL
  virtual bool GetUrlComponents(
      const std::string& url,
      UrlComponents* components) const = 0;

  // Log an event
  virtual void EventLog(const std::string& json) = 0;

  // Logs debug information
  virtual std::unique_ptr<LogStream> Log(
      const char* file,
      int line,
      const LogLevel log_level) const = 0;
};

}  // namespace ads
