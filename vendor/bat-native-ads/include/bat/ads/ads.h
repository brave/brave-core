/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ADS_H_
#define BAT_ADS_ADS_H_

#include <string>

#include "bat/ads/ads_client.h"
#include "bat/ads/export.h"
#include "bat/ads/notification_result_type.h"
#include "bat/ads/notification_info.h"

namespace ads {

// Reduces the wait time before calling the StartCollectingActivity function
extern bool _is_debug;

// Easter egg for serving Ads every kNextEasterEggStartsInSeconds seconds. The
// user must visit www.iab.com and the manually refresh the page to serve the
// next easter egg
extern bool _is_testing;

// Determines whether to use the staging or production Ad Serve
extern bool _is_production;

extern const char _bundle_schema_name[];
extern const char _catalog_schema_name[];
extern const char _catalog_name[];
extern const char _client_name[];

class ADS_EXPORT Ads {
 public:
  Ads() = default;
  virtual ~Ads() = default;

  static Ads* CreateInstance(AdsClient* ads_client);

  // Should be called to determine if Ads are supported for the specified locale
  static bool IsSupportedRegion(const std::string& locale);

  // Should be called when Ads are enabled or disabled on the Client
  virtual void Initialize() = 0;

  // Should be called when the browser enters the foreground
  virtual void OnForeground() = 0;

  // Should be called when the browser enters the background
  virtual void OnBackground() = 0;

  // Should be called periodically on desktop browsers as set by
  // SetIdleThreshold to record when the browser is idle. This call is optional
  // for mobile devices
  virtual void OnIdle() = 0;

  // Should be called periodically on desktop browsers as set by
  // SetIdleThreshold to record when the browser is no longer idle. This call is
  // optional for mobile devices
  virtual void OnUnIdle() = 0;

  // Should be called to record when a tab has started playing media (A/V)
  virtual void OnMediaPlaying(const int32_t tab_id) = 0;

  // Should be called to record when a tab has stopped playing media (A/V)
  virtual void OnMediaStopped(const int32_t tab_id) = 0;

  // Should be called to record user activity on a browser tab
  virtual void TabUpdated(
      const int32_t tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_incognito) = 0;

  // Should be called to record when a browser tab is closed
  virtual void TabClosed(const int32_t tab_id) = 0;

  // Should be called to remove all cached history
  virtual void RemoveAllHistory() = 0;

  // Should be called to inform Ads if Confirmations is ready
  virtual void SetConfirmationsIsReady(const bool is_ready) = 0;

  // Should be called when the user changes the operating system's locale, i.e.
  // en, en_US or en_GB.UTF-8 unless the operating system restarts the app
  virtual void ChangeLocale(const std::string& locale) = 0;

  // Should be called when a page has loaded in the current browser tab, and the
  // HTML is available for analysis
  virtual void ClassifyPage(
      const std::string& url,
      const std::string& html) = 0;

  // Should be called when the user invokes "Show Sample Ad" on the Client; a
  // Notification is then sent to the Client for processing
  virtual void ServeSampleAd() = 0;

  // Should be called when a timer is triggered
  virtual void OnTimer(const uint32_t timer_id) = 0;

  // Should be called when a Notification has been shown
  virtual void GenerateAdReportingNotificationShownEvent(
      const NotificationInfo& info) = 0;

  // Should be called when a Notification has been clicked, dismissed or times
  // out on the Client. Dismiss events for local Notifications may not be
  // available for every version of Android, making the Dismiss notification
  // capture optional for Android on 100% of devices
  virtual void GenerateAdReportingNotificationResultEvent(
      const NotificationInfo& info,
      const NotificationResultInfoResultType type) = 0;

 private:
  // Not copyable, not assignable
  Ads(const Ads&) = delete;
  Ads& operator=(const Ads&) = delete;
};

}  // namespace ads

#endif  // BAT_ADS_ADS_H_
