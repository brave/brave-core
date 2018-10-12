/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "../include/ads_client.h"
#include "../include/export.h"

namespace ads {

extern bool is_production;
extern bool is_verbose;

ADS_EXPORT class Ads {
 public:
  Ads() = default;
  virtual ~Ads() = default;

  // Not copyable, not assignable
  Ads(const Ads&) = delete;
  Ads& operator=(const Ads&) = delete;

  static Ads* CreateInstance(AdsClient* ads_client);

  Ads* ads_;  // NOT OWNED

  // Initialize
  virtual void Initialize() = 0;

  // Called whenever the browser gains or loses focus (the active application)
  virtual void AppFocused(const bool focused) = 0;

  // Called to record user activity on a tab
  virtual void TabUpdate() = 0;

  // Called to record when a user is no longer idle
  virtual void RecordUnIdle() = 0;

  // Called to remove all cached history
  virtual void RemoveAllHistory() = 0;

  // Called when the browser is about to exit, if Brave Ads
  // is not enabled, then removes all userModelState
  virtual void SaveCachedInfo() = 0;

  // Called to schedule network activity for talking to the catalog
  // and/or redemption servers
  virtual void ConfirmAdUUIDIfAdEnabled() = 0;

  // Called to determine if a URL is a shopping site and
  // update userModelState accordingly
  virtual void TestShoppingData(const std::string& url) = 0;

  // Called to determine if a URL is a search result and update
  // userModelState accordingly
  virtual void TestSearchState(const std::string& url) = 0;

  // Called to record whenever a tab is playing (or has
  // stopped playing) media (A/V)
  virtual void RecordMediaPlaying(
      const std::string& tabId,
      const bool active) = 0;

  // Called when the user changes if notifications are available
  virtual void ChangeNotificationsAvailable(const bool available) = 0;

  // Called when the user changes if notifications are allowed
  virtual void ChangeNotificationsAllowed(const bool allowed) = 0;

  // Called when a page is completely loaded and both the headers
  // and body are available for analysis
  virtual void ClassifyPage(const std::string& page) = 0;

  // Called when the user changes their locale (e.g., "en", "fr", or "gb")
  virtual void ChangeLocale(const std::string& locale) = 0;

  // Frequently called when it may be time to talk to
  // the catalog and/or redemption server
  virtual void CollectActivity() = 0;

  // Called when the catalog server has returned a result.
  // If the result is good, an upcall is made to save the catalog state
  // and save the userModel state
  virtual void ApplyCatalog() = 0;

  // Called to get the network SSID
  virtual void RetrieveSSID() = 0;

  // Frequently called to determine whether a notification should be
  // displayed; if so, the notification is sent
  virtual void CheckReadyAdServe(const bool forced) = 0;

  // Called when the user invokes "Show Sample Ad"
  virtual void ServeSampleAd() = 0;

  // Called when a timer is triggered
  virtual void OnTimer(const uint32_t timer_id) = 0;

  // Called once settings state has loaded
  virtual void OnSettingsStateLoaded(
      const Result result,
      const std::string& json) = 0;

  // Called once user model state has been saved
  virtual void OnUserModelStateSaved(const Result result) = 0;

  // Called once user model state has loaded
  virtual void OnUserModelStateLoaded(
      const Result result,
      const std::string& json) = 0;

  // Called once catalog state has been saved
  virtual void OnCatalogStateSaved(const Result result) = 0;

  // Called once catalog state has loaded
  virtual void OnCatalogStateLoaded(
      const Result result,
      const std::string& json) = 0;
};

}  // namespace ads
