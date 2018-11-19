/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "bat/ads/ads_client.h"
#include "bat/ads/export.h"
#include "bat/ads/notification_result_type.h"
#include "bat/ads/notification_info.h"

namespace ads {

extern bool _is_debug;
extern bool _is_testing;
extern bool _is_production;

class ADS_EXPORT Ads {
 public:
  Ads() = default;
  virtual ~Ads() = default;

  // Not copyable, not assignable
  Ads(const Ads&) = delete;
  Ads& operator=(const Ads&) = delete;

  static Ads* CreateInstance(AdsClient* ads_client);

  // Should be called when a notification has been shown on the Client
  virtual void GenerateAdReportingNotificationShownEvent(
      const NotificationInfo& info) = 0;

  // Should be called when a notification has been clicked, dismissed or times
  // out on the Client
  virtual void GenerateAdReportingNotificationResultEvent(
      const NotificationInfo& info,
      const NotificationResultInfoResultType type) = 0;

  // Should be called when ads are enabled or disabled on the Client
  virtual void Initialize() = 0;

  // Should be called whenever the browser enters the foreground
  virtual void OnForeground() = 0;

  // Should be called whenever the browser enters the background
  virtual void OnBackground() = 0;

  // Should be called to record when the browser is idle
  virtual void OnIdle() = 0;

  // Should be called to record when the browser is no longer idle
  virtual void OnUnIdle() = 0;

  // Should be called to record when a tab has started playing media (A/V)
  virtual void OnMediaPlaying(const int32_t tab_id) = 0;

  // Should be called to record when a tab has stopped playing media (A/V)
  virtual void OnMediaStopped(const int32_t tab_id) = 0;

  // Should be called to record user activity on a tab
  virtual void TabUpdated(
      const int32_t tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_incognito) = 0;

  // Should be called to record when a user closes a tab
  virtual void TabClosed(const int32_t& tab_id) = 0;

  // Should be called to remove all cached history
  virtual void RemoveAllHistory() = 0;

  // Should be called when the browser is about to exit; if ads are disabled
  // the client state is reset to default values
  virtual void SaveCachedInfo() = 0;

  // Should be called when a page is completely loaded and the body is available
  // for analysis
  virtual void ClassifyPage(
      const std::string& url,
      const std::string& html) = 0;

  // Should be called when the user changes their device locale (e.g., "en_US",
  // "fr", or "en" etc.)
  virtual void ChangeLocale(const std::string& locale) = 0;

  // Frequently called to determine whether a notification should be displayed;
  // if so, the notification is sent to the Client for processing
  virtual void CheckReadyAdServe(const bool forced = false) = 0;

  // Should be called when the user invokes "Show Sample Ad"; a notification is
  // sent to the client for processing
  virtual void ServeSampleAd() = 0;

  // Should be called when a timer is triggered
  virtual void OnTimer(const uint32_t timer_id) = 0;
};

}  // namespace ads
