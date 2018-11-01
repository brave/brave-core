/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "ads_client.h"
#include "event_type_notification_shown_info.h"
#include "event_type_notification_result_info.h"
#include "event_type_sustain_info.h"
#include "export.h"

namespace ads {

extern bool _is_testing;
extern bool _is_production;
extern bool _is_verbose;

ADS_EXPORT class Ads {
 public:
  Ads() = default;
  virtual ~Ads() = default;

  // Not copyable, not assignable
  Ads(const Ads&) = delete;
  Ads& operator=(const Ads&) = delete;

  static Ads* CreateInstance(AdsClient* ads_client);

  virtual void GenerateAdReportingNotificationShownEvent(
      const event_type::NotificationShownInfo& info) = 0;

  virtual void GenerateAdReportingNotificationResultEvent(
      const event_type::NotificationResultInfo& info) = 0;

  virtual void GenerateAdReportingSustainEvent(
      const event_type::SustainInfo& info) = 0;

  // Initialize
  virtual void Initialize() = 0;

  // Initialize
  virtual void InitializeUserModel(const std::string& json) = 0;

  // Called whenever the browser gains or loses focus (the active application)
  virtual void AppFocused(const bool focused) = 0;

  // Called to record user activity on a tab
  virtual void TabUpdated(
      const std::string& tab_id,
      const std::string& url,
      const bool active,
      const bool incognito) = 0;

  // Called to record when a user switches tab
  virtual void TabSwitched(
      const std::string& tab_id,
      const std::string& url,
      const bool incognito) = 0;

  // Called to record when a user closes a tab
  virtual void TabClosed(const std::string& tab_id) = 0;

  // Called to record when a user is no longer idle
  virtual void RecordUnIdle() = 0;

  // Called to remove all cached history
  virtual void RemoveAllHistory() = 0;

  // Called when the browser is about to exit, if Brave Ads
  // is not enabled, then removes all client state
  virtual void SaveCachedInfo() = 0;

  // Called to schedule network activity for talking to the catalog
  // and/or redemption servers
  virtual void ConfirmAdUUIDIfAdEnabled() = 0;

  // Called to determine if a URL is a shopping site and
  // update the client state accordingly
  virtual void TestShoppingData(const std::string& url) = 0;

  // Called to determine if a URL is a search result and update
  // client state accordingly
  virtual void TestSearchState(const std::string& url) = 0;

  // Called to record whenever a tab is playing (or has
  // stopped playing) media (A/V)
  virtual void RecordMediaPlaying(
      const std::string& tab_id,
      const bool active) = 0;

  // Called when a page is completely loaded and both the headers
  // and body are available for analysis
  virtual void ClassifyPage(const std::string& html) = 0;

  // Called when the user changes their locale (e.g., "en", "fr", or "gb")
  virtual void ChangeLocale(const std::string& locale) = 0;

  // Frequently called when it may be time to talk to
  // the catalog and/or redemption server
  virtual void CollectActivity() = 0;

  // Called when the catalog server has returned a result.
  // If the result is good, an upcall is made to save the catalog state
  // and save the client state
  virtual void ApplyCatalog() = 0;

  // Called to get the network SSID
  virtual void RetrieveSSID() = 0;

  // Frequently called to determine whether a notification should be
  // displayed; if so, the notification is sent
  virtual void CheckReadyAdServe(const bool forced = false) = 0;

  // Called when the user invokes "Show Sample Ad"
  virtual void ServeSampleAd() = 0;

  // Called to flag whether notifications are available
  virtual void SetNotificationsAvailable(const bool available) = 0;

  // Called to flag whether notifications are allowed
  virtual void SetNotificationsAllowed(const bool allowed) = 0;

  // Called to flag whether notifications are configured
  virtual void SetNotificationsConfigured(const bool configured) = 0;

  // Called to flag whether notifications have expired
  virtual void SetNotificationsExpired(const bool expired) = 0;

  // Called when a timer is triggered
  virtual void OnTimer(const uint32_t timer_id) = 0;

  // Called once the user model has loaded
  virtual void OnUserModelLoaded(const Result result) = 0;

  // Called once settings have loaded
  virtual void OnSettingsLoaded(
      const Result result,
      const std::string& json) = 0;

  // Called once client has been saved
  virtual void OnClientSaved(const Result result) = 0;

  // Called once client has loaded
  virtual void OnClientLoaded(
      const Result result,
      const std::string& json) = 0;

  // Called once bundle has been saved
  virtual void OnBundleSaved(const Result result) = 0;

  // Called once bundle has loaded
  virtual void OnBundleLoaded(
      const Result result,
      const std::string& json) = 0;

  virtual void OnGetSampleCategory(
      const ads::Result result,
      const std::string& category) = 0;

  // Called after getting ads
  virtual void OnGetAds(
      const Result result,
      const std::string& category,
      const std::vector<bundle::CategoryInfo>& ads) = 0;
};

}  // namespace ads
