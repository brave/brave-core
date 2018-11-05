/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "bat/ads/ads_client.h"
#include "bat/ads/event_type_notification_shown_info.h"
#include "bat/ads/event_type_notification_result_info.h"
#include "bat/ads/event_type_sustain_info.h"
#include "bat/ads/export.h"

namespace ads {

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

  // Should be called when a notification has been shown
  virtual void GenerateAdReportingNotificationShownEvent(
      const NotificationShownInfo& info) = 0;

  // Should be called when a notification has been clicked, dismissed or times
  // out
  virtual void GenerateAdReportingNotificationResultEvent(
      const NotificationResultInfo& info) = 0;

  // Should be called when a notification has been viewed for an extended period
  // without interruption
  virtual void GenerateAdReportingSustainEvent(
      const SustainInfo& info) = 0;

  // Should be called to initialize ads
  virtual void Initialize() = 0;

  // Should be called to initialize the user model
  virtual void InitializeUserModel(const std::string& json) = 0;

  // Should be called whenever the browser gains or loses focus
  virtual void AppFocused(const bool is_focused) = 0;

  // Should be called to record user activity on a tab
  virtual void TabUpdated(
      const std::string& tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_incognito) = 0;

  // Should be called to record when a user switches tab
  virtual void TabSwitched(
      const std::string& tab_id,
      const std::string& url,
      const bool is_incognito) = 0;

  // Should be called to record when a user closes a tab
  virtual void TabClosed(const std::string& tab_id) = 0;

  // Should be called to record when a user is no longer idle
  virtual void RecordUnIdle() = 0;

  // Should be called to remove all cached history
  virtual void RemoveAllHistory() = 0;

  // Should be called when the browser is about to exit, if ads is not enabled,
  // then removes all client state
  virtual void SaveCachedInfo() = 0;

  // Should be called to record whenever a tab is playing (or has stopped
  // playing) media (A/V)
  virtual void RecordMediaPlaying(
      const std::string& tab_id,
      const bool is_playing) = 0;

  // Should be called when a page is completely loaded and the body is available
  // for analysis
  virtual void ClassifyPage(
      const std::string& url,
      const std::string& html) = 0;

  // Should be called when the user changes their locale (e.g., "en_US", "fr",
  // or "gb")
  virtual void ChangeLocale(const std::string& locale) = 0;

  // Frequently called to determine whether a notification should be displayed;
  // if so, the notification is sent
  virtual void CheckReadyAdServe(const bool forced = false) = 0;

  // Should be called when the user invokes "Show Sample Ad"
  virtual void ServeSampleAd() = 0;

  // Should be called to flag whether notifications are available
  virtual void SetNotificationsAvailable(const bool available) = 0;

  // Should be called to flag whether notifications are allowed
  virtual void SetNotificationsAllowed(const bool allowed) = 0;

  // Should be called to flag whether notifications are configured
  virtual void SetNotificationsConfigured(const bool configured) = 0;

  // Should be called to flag whether notifications have expired
  virtual void SetNotificationsExpired(const bool expired) = 0;

  // Should be called when a timer is triggered
  virtual void OnTimer(const uint32_t timer_id) = 0;

  // Should be called once the user model has loaded
  virtual void OnUserModelLoaded(const Result result) = 0;

  // Should be called once settings have loaded
  virtual void OnSettingsLoaded(
      const Result result,
      const std::string& json) = 0;

  // Should be called once the client has been saved
  virtual void OnClientSaved(const Result result) = 0;

  // Should be called once the client has loaded
  virtual void OnClientLoaded(
      const Result result,
      const std::string& json) = 0;

  // Should be called once the bundle has been saved
  virtual void OnBundleSaved(const Result result) = 0;

  // Should be called once the bundle has loaded
  virtual void OnBundleLoaded(
      const Result result,
      const std::string& json) = 0;

  // Should be Called after getting a sample category
  virtual void OnGetSampleCategory(
      const Result result,
      const std::string& category) = 0;

  // Should be called after getting ads
  virtual void OnGetAds(
      const Result result,
      const std::string& category,
      const std::vector<CategoryInfo>& ads) = 0;
};

}  // namespace ads
