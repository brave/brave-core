/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ADS_CLIENT_H_
#define BAT_ADS_ADS_CLIENT_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <ostream>
#include <memory>
#include <functional>

#include "bat/ads/creative_ad_notification_info.h"
#include "bat/ads/creative_publisher_ad_info.h"
#include "bat/ads/ad_conversion_tracking_info.h"
#include "bat/ads/issuers_info.h"
#include "bat/ads/bundle_state.h"
#include "bat/ads/client_info.h"
#include "bat/ads/export.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/publisher_ad_info.h"
#include "bat/ads/result.h"

namespace ads {

enum LogLevel {
  LOG_ERROR = 1,
  LOG_WARNING,
  LOG_INFO
};

enum URLRequestMethod {
  GET = 0,
  PUT = 1,
  POST = 2
};

class ADS_EXPORT LogStream {
 public:
  virtual ~LogStream() = default;
  virtual std::ostream& stream() = 0;
};

using OnSaveCallback = std::function<void(const Result)>;
using OnLoadCallback = std::function<void(const Result, const std::string&)>;

using OnResetCallback = std::function<void(const Result)>;

using OnGetCreativeAdNotificationsCallback = std::function<void(const Result,
    const std::vector<std::string>&, const CreativeAdNotifications&)>;

using OnGetCreativePublisherAdsCallback = std::function<void(const Result,
    const std::string&, const std::vector<std::string>&,
        const std::vector<std::string>&, const CreativePublisherAds&)>;

using OnGetAdConversionsCallback = std::function<void(const Result,
    const std::string&, const AdConversions&)>;

using OnLoadSampleBundleCallback = std::function<void(const Result,
    const std::string&)>;

using URLRequestCallback = std::function<void(const int, const std::string&,
    const std::map<std::string, std::string>&)>;

class ADS_EXPORT AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Should return |true| if ads is enabled; otherwise, should return |false|
  virtual bool IsEnabled() const = 0;

  // Should return |true| if publisher ads should be shown on participating
  // sites; otherwise, should return |false|
  virtual bool ShouldShowPublisherAdsOnPariticipatingSites() const = 0;

  // Should return |true| if allow ad conversion tracking is enabled; otherwise,
  // should return |false|
  virtual bool ShouldAllowAdConversionTracking() const = 0;

  // Should return the locale of the operating system using one of the following
  // formats:
  //
  //     <language>-<REGION> i.e. en-US
  //     <language>-<REGION>.<ENCODING> i.e. en-US.UTF-8
  //     <language>_<REGION> i.e. en_US
  //     <language>-<REGION>.<ENCODING> i.e. en_US.UTF-8
  virtual const std::string GetLocale() const = 0;

  // Should return the maximum number of ads that can be shown per hour
  virtual uint64_t GetAdsPerHour() const = 0;

  // Should return the maximum number of ads that can be shown per day
  virtual uint64_t GetAdsPerDay() const = 0;

  // Set the idle threshold specified in seconds for how long a user should be
  // idle before |OnUnIdle| is called. This call is optional for mobile devices
  virtual void SetIdleThreshold(
      const int threshold) = 0;

  // Should return |true| if there is an available network connection;
  // otherwise, should return |false|
  virtual bool IsNetworkConnectionAvailable() const = 0;

  // Should get information about the client, i.e. Platform. returned in |info|
  virtual void GetClientInfo(
      ClientInfo* info) const = 0;

  // Should return an array of supported User Model languages
  virtual const std::vector<std::string>
      GetUserModelLanguages() const = 0;

  // Should load the User Model for the specified language, user models are a
  // dependency of the application and should be bundled accordingly, the
  // following file structure should be used:
  //
  //   resources/
  //   ├── languages/
  //   ├──── de/
  //   │     ├── user_model.json
  //   ├──── en/
  //   │     ├── user_model.json
  //   ├──── fr/
  //   │     └── user_model.json
  //
  // For information on |user_model.json| and the BAT Native User Model see
  // https://github.com/brave-intl/bat-native-usermodel/blob/master/README.md
  virtual void LoadUserModelForLanguage(
      const std::string& language,
      OnLoadCallback callback) const = 0;

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
      const std::string& id) = 0;

  // Should pass-through to Confirmations that the catalog issuers have changed
  virtual void SetCatalogIssuers(
      const std::unique_ptr<IssuersInfo> info) = 0;

  // Should pass-through to Confirmations that an ad notification was viewed,
  // clicked, dismissed or landed
  virtual void ConfirmAdNotification(
      const std::unique_ptr<AdNotificationInfo> info) = 0;

  // Should pass-through to Confirmations that a publisher ad was viewed,
  // clicked or landed
  virtual void ConfirmPublisherAd(
      const PublisherAdInfo& info) = 0;

  // Should pass-through to Confirmations that an ad was flagged, upvoted or
  // downvoted
  virtual void ConfirmAction(
      const std::string& uuid,
      const std::string& creative_set_id,
      const ConfirmationType& type) = 0;

  // Should create a timer to trigger after the time offset specified in
  // seconds. If the timer was created successfully a unique identifier should
  // be returned, otherwise returns |0|
  virtual uint32_t SetTimer(
      const uint64_t time_offset) = 0;

  // Should destroy the timer associated with the specified timer identifier
  virtual void KillTimer(
      const uint32_t timer_id) = 0;

  // Should fetch and return data. Loading should be performed asynchronously,
  // so that the app remains responsive and should handle incoming data or
  // errors as they arrive. The callback takes 4 arguments — |url| should
  // contain the Uniform Resource Locator. |response_status_code| should convey
  // the result of the request. |response| should contain the HTTP response
  // message. |headers| should contain the HTTP headers.
  virtual void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLRequestMethod method,
      URLRequestCallback callback) = 0;

  // Should save a value to persistent storage. The callback takes one argument
  // — |Result| should be set to |SUCCESS| if successful; otherwise, should be
  // set to |FAILED|
  virtual void Save(
      const std::string& name,
      const std::string& value,
      OnSaveCallback callback) = 0;

  // Should save the bundle state to persistent storage. The callback takes one
  // argument — |Result| should be set to |SUCCESS| if successful; otherwise,
  // should be set to |FAILED|
  virtual void SaveBundleState(
      std::unique_ptr<BundleState> state,
      OnSaveCallback callback) = 0;

  // Should load a value from persistent storage. The callback takes 2 arguments
  // — |Result| should be set to |SUCCESS| if successful; otherwise, should be
  // set to |FAILED|. |value| should contain the persisted value
  virtual void Load(
      const std::string& name, OnLoadCallback callback) = 0;

  // Should load a JSON schema from persistent storage, schemas are a dependency
  // of the application and should be bundled accordingly, the following file
  // structure should be used:
  //
  //   resources/
  //   ├──catalog-schema.json
  //   ├──bundle-schema.json
  //
  // |catalog-schema.json| and |bundle-schema.json| are JSON schemas which
  // specify the JSON-based format to define the structure of the JSON data for
  // validation, documentation, and interaction control. It provides the
  // contract for the JSON data and how that data can be modified
  virtual const std::string LoadJsonSchema(
      const std::string& name) = 0;

  // Should load the sample bundle from persistent storage. The callback takes 2
  // arguments — |Result| should be set to |SUCCESS| if successful; otherwise,
  // should be set to |FAILED|. |value| should contain the sample bundle
  virtual void LoadSampleBundle(
      OnLoadSampleBundleCallback callback) = 0;

  // Should reset a previously persisted value. The callback takes one argument
  // — |Result| should be set to |SUCCESS| if successful; otherwise, should be
  // set to |FAILED|
  virtual void Reset(
      const std::string& name, OnResetCallback callback) = 0;

  // Should fetch all creative ad notifications for the specified |category|
  // where the current time is between the ad |start_timestamp| and
  // |end_timestamp| from the previously persisted bundle state. The callback
  // takes 3 arguments — |Result| should be set to |SUCCESS| if successful;
  // otherwise, should be set to |FAILED|. |category| should contain the
  // category. |ads| should contain an array of ads
  virtual void GetCreativeAdNotifications(
      const std::vector<std::string>& categories,
      OnGetCreativeAdNotificationsCallback callback) = 0;

  // Should fetch all creative publisher ads for the specified |url|,
  // |categories| and |sizes|. The callback takes 5 arguments — |Result| should
  // be set to |SUCCESS| if successful; otherwise, should be set to |FAILED|.
  // |url| should contain the requested URL. |categories| should contain the
  // requested categories. |sizes| should contain the requested sizes. |ads|
  // should contain an array of ads
  virtual void GetCreativePublisherAds(
      const std::string& url,
      const std::vector<std::string>& categories,
      const std::vector<std::string>& sizes,
      const OnGetCreativePublisherAdsCallback callback) = 0;

  // Should fetch all ad conversions for the specified |url| from the previously
  // persisted bundle state. The callback takes 3 arguments — |Result| should be
  // set to |SUCCESS| if successful; otherwise, should be set to |FAILED|. |url|
  // should contain the url. |ad_conversions| should contain an array of ad
  // conversions
  virtual void GetAdConversions(
      const std::string& url,
      OnGetAdConversionsCallback callback) = 0;

  // Should log an event
  virtual void EventLog(
      const std::string& json) const = 0;

  // Should log diagnostic information
  virtual std::unique_ptr<LogStream> Log(
      const char* file,
      const int line,
      const LogLevel log_level) const = 0;
};

}  // namespace ads

#endif  // BAT_ADS_ADS_CLIENT_H_
