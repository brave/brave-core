/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ADS_H_
#define BAT_ADS_ADS_H_

#include <stdint.h>
#include <map>
#include <string>
#include <memory>
#include <vector>

#include "bat/ads/ad_content.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/category_content.h"
#include "bat/ads/export.h"
#include "bat/ads/notification_event_type.h"
#include "bat/ads/notification_info.h"

namespace ads {

struct AdsHistory;

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

using InitializeCallback = std::function<void(const Result)>;
using ShutdownCallback = std::function<void(const Result)>;
using GetNotificationForIdCallback =
    std::function<void(std::unique_ptr<NotificationInfo>)>;
using RemoveAllHistoryCallback = std::function<void(const Result)>;

class ADS_EXPORT Ads {
 public:
  Ads() = default;
  virtual ~Ads() = default;

  static Ads* CreateInstance(AdsClient* ads_client);

  // Should be called to determine if Ads are supported for the specified locale
  static bool IsSupportedRegion(const std::string& locale);

  // Should be called to get the region for the specified locale
  static std::string GetRegion(const std::string& locale);

  // Should be called when Ads is enabled on the Client
  virtual void Initialize(InitializeCallback callback) = 0;

  // Should be called when Ads is disabled on the Client
  virtual void Shutdown(ShutdownCallback callback) = 0;

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

  // Should be called periodically on desktop browsers as set by
  // SetIdleThreshold to record when the browser is no longer idle. This call is
  // optional for mobile devices
  virtual void OnUnIdle() = 0;

  // Should be called periodically on desktop browsers as set by
  // SetIdleThreshold to record when the browser is idle. This call is optional
  // for mobile devices
  virtual void OnIdle() = 0;

  // Should be called when the browser enters the foreground
  virtual void OnForeground() = 0;

  // Should be called when the browser enters the background
  virtual void OnBackground() = 0;

  // Should be called to record when a tab has started playing media (A/V)
  virtual void OnMediaPlaying(const int32_t tab_id) = 0;

  // Should be called to record when a tab has stopped playing media (A/V)
  virtual void OnMediaStopped(const int32_t tab_id) = 0;

  // Should be called to record user activity on a browser tab
  virtual void OnTabUpdated(
      const int32_t tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_incognito) = 0;

  // Should be called to record when a browser tab is closed
  virtual void OnTabClosed(const int32_t tab_id) = 0;

  // Should return true and NotificationInfo if the notification for the
  // specified id exists otherwise returns false
  virtual bool GetNotificationForId(
      const std::string& id,
      NotificationInfo* notification) = 0;

  // Should be called when a notification event is triggered
  virtual void OnNotificationEvent(
      const std::string& id,
      const NotificationEventType type) = 0;

  // Should be called to remove all cached history
  virtual void RemoveAllHistory(RemoveAllHistoryCallback callback) = 0;

  // Should be called to retrieve ads history
  virtual std::map<uint64_t, std::vector<AdsHistory>> GetAdsHistory() = 0;

  // Should be called to indicate interest in the given ad. This is a
  // toggle, so calling it again returns the setting to the neutral
  // state
  virtual AdContent::LikeAction ToggleAdThumbUp(
      const std::string& id,
      const std::string& creative_set_id,
      AdContent::LikeAction action) = 0;

  // Should be called to indicate a lack of interest in the given
  // ad. This is a toggle, so calling it again returns the setting to
  // the neutral state
  virtual AdContent::LikeAction ToggleAdThumbDown(
      const std::string& id,
      const std::string& creative_set_id,
      AdContent::LikeAction action) = 0;

  // Should be called to opt-in to the given ad category. This is a
  // toggle, so calling it again returns the setting to the neutral
  // state
  virtual CategoryContent::OptAction ToggleAdOptInAction(
      const std::string& category,
      CategoryContent::OptAction action) = 0;

  // Should be called to opt-out of the given ad category. This is a
  // toggle, so calling it again returns the setting to the neutral
  // state
  virtual CategoryContent::OptAction ToggleAdOptOutAction(
      const std::string& category,
      CategoryContent::OptAction action) = 0;

  // Should be called to save an ad for later viewing. This is a
  // toggle, so calling it again removes the ad from the saved list
  virtual bool ToggleSaveAd(const std::string& id,
                            const std::string& creative_set_id,
                            bool saved) = 0;

  // Should be called to flag an ad as inappropriate. This is a
  // toggle, so calling it again unflags the ad
  virtual bool ToggleFlagAd(const std::string& id,
                            const std::string& creative_set_id,
                            bool flagged) = 0;

 private:
  // Not copyable, not assignable
  Ads(const Ads&) = delete;
  Ads& operator=(const Ads&) = delete;
};

}  // namespace ads

#endif  // BAT_ADS_ADS_H_
