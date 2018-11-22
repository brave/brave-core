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

  // Should return true if ads are enabled otherwise false
  virtual bool IsAdsEnabled() const = 0;

  // Should return the operating systems locale, i.e. en, en_US or en_GB.UTF-8
  virtual const std::string GetAdsLocale() const = 0;

  // Should return the amount of ads that can be shown per hour
  virtual uint64_t GetAdsPerHour() const = 0;

  // Should return the amount of ads that can be shown per day
  virtual uint64_t GetAdsPerDay() const = 0;

  // Sets the idle threshold, specified in seconds for how often OnIdle or
  // OnUndle should be called
  virtual void SetIdleThreshold(const int threshold) = 0;

  // Should return true if there is a network connection otherwise false
  virtual bool IsNetworkConnectionAvailable() = 0;

  // Should return information about the client
  virtual void GetClientInfo(ClientInfo* info) const = 0;

  // Should return a list of supported User Model locales
  virtual const std::vector<std::string> GetLocales() const = 0;

  // Should load the User Model for the specified locale, user models are a
  // dependency of the application and should be bundled accordingly, the
  // following file structure could be used:
  //
  // locales/
  // ├── de/
  // │   └── user_model.json
  // ├── en/
  // │   └── user_model.json
  // ├── fr/
  // │   └── user_model.json

  virtual void LoadUserModelForLocale(
      const std::string& locale,
      OnLoadCallback callback) const = 0;

  // Should generate return a v4 UUID
  virtual const std::string GenerateUUID() const = 0;

  // Should return the network SSID or an empty string if not available
  virtual const std::string GetSSID() const = 0;

  // Should return true if the operating system supports notifications otherwise
  // false
  virtual bool IsNotificationsAvailable() const = 0;

  // Should show a notification
  virtual void ShowNotification(std::unique_ptr<NotificationInfo> info) = 0;

  // Should create a timer to tigger after the time offset specified in seconds.
  // If the timer was created a timer id should be returned, otherwise return 0
  virtual uint32_t SetTimer(const uint64_t& time_offset) = 0;

  // Should destroy the timer associated with the timer id
  virtual void KillTimer(uint32_t timer_id) = 0;

  // Should start a URL request
  virtual void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      URLRequestMethod method,
      URLRequestCallback callback) = 0;

  // Should save a value to persistent storage
  virtual void Save(
      const std::string& name,
      const std::string& value,
      OnSaveCallback callback) = 0;

  // Should save the bundle state to persistent storage
  virtual void SaveBundleState(
    std::unique_ptr<BundleState> state,
    OnSaveCallback callback) = 0;

  // Should load a value from persistent storage
  virtual void Load(const std::string& name, OnLoadCallback callback) = 0;

  // Should load a JSON schema from persistent storage
  virtual const std::string LoadJsonSchema(const std::string& name) = 0;

  // Should load the sample bundle from persistent storage
  virtual void LoadSampleBundle(OnLoadSampleBundleCallback callback) = 0;

  // Should reset a previously saved value, i.e. remove the file from persistent
  // storage
  virtual void Reset(const std::string& name, OnResetCallback callback) = 0;

  // Should get ads for the specified region and category from the previously
  // persisted bundle state
  virtual void GetAds(
      const std::string& region,
      const std::string& category,
      OnGetAdsCallback callback) = 0;

  // Should get the components of a URL
  virtual bool GetUrlComponents(
      const std::string& url,
      UrlComponents* components) const = 0;

  // Should log an event to persistent storage however as events may be queued
  // they need an event name and timestamp adding as follows, replacing ... with
  // the value of the json parameter:
  //
  // {
  //   "time": "2018-11-19T15:47:43.634Z",
  //   "eventName": "Event logged",
  //   ...
  // }
  virtual void EventLog(const std::string& json) = 0;

  // Should log diagnostic information to persistent storage
  virtual std::ostream& Log(
      const char* file,
      int line,
      const LogLevel log_level) const = 0;
};

}  // namespace ads
