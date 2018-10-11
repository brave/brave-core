/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>

#include "../include/callback_handler.h"
#include "../include/ads_url_loader.h"
#include "../include/catalog_campaign.h"
#include "../include/catalog_creative_set.h"

namespace ads {

enum URL_METHOD {
  GET = 0,
  PUT = 1,
  POST = 2
};

using CampaignInfoCallback = std::function<void (Result,
    std::unique_ptr<catalog::CampaignInfo>)>;

using CreativeSetInfoCallback = std::function<void (Result,
    std::unique_ptr<catalog::CreativeSetInfo>)>;

class AdsClient {
 public:
  virtual ~AdsClient() = default;

  virtual void LoadState(CallbackHandler* handler) = 0;
  virtual void SaveState(const std::string& ads_state,
      CallbackHandler* handler) = 0;

  virtual void SaveCampaignInfo(std::unique_ptr<catalog::CampaignInfo> info,
      CampaignInfoCallback callback) = 0;
  virtual void LoadCampaignInfo(catalog::CampaignInfoFilter filter,
      CampaignInfoCallback callback) = 0;

  virtual void SaveCreativeSetInfo(std::unique_ptr<catalog::CreativeSetInfo>
      info, CreativeSetInfoCallback callback) = 0;
  virtual void LoadCreativeSetInfo(catalog::CreativeSetInfoFilter filter,
      CreativeSetInfoCallback callback) = 0;

  // Initialize
  virtual void Initialize() = 0;

  // Called whenever the browser gains or loses focus (the active application)
  virtual void AppFocused(bool focused) = 0;

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
  virtual void ConfirmAdUUIDIfAdEnabled(bool enabled) = 0;

  // Called to determine if a URL is a shopping site and
  // update userModelState accordingly
  virtual void TestShoppingData(const std::string& url) = 0;

  // Called to determine if a URL is a search result and update
  // userModelState accordingly
  virtual void TestSearchState(const std::string& url) = 0;

  // Called to record whenever a tab is playing(or has
  // stopped playing) media(A / V)
  virtual void RecordMediaPlaying(bool active, uint64_t tabId) = 0;

  // Called when a page is completely loaded and both the headers
  // and body is available for analysis
  virtual void ClassifyPage(uint64_t windowId) = 0;

  // Called when the user changes their locale(e.g., "en", "fr", or "gb")
  virtual void ChangeLocale(const std::string& locale) = 0;

  // Frequently called when it may be time to talk to
  // the catalog and/or redemption server
  virtual void CollectActivity() = 0;

  // Called when the cached version of the catalog
  // is loaded on browser start - up)
  virtual void InitializeCatalog() = 0;

  // Called to retrieve the network SSID
  virtual void RetrieveSSID(uint64_t error, const std::string& ssid) = 0;

  // Frequently called to determine whether a notification should be
  // displayed; if so, the notification is returned
  virtual void CheckReadyAdServe(uint64_t windowId, bool forceP) = 0;

  // Called when the user invokes "Show Sample Ad"
  virtual void ServeSampleAd(uint64_t windowId) = 0;

  // uint64_t time_offset (input): timer offset in seconds
  // uint32_t timer_id (output): 0 in case of failure
  virtual void SetTimer(uint64_t time_offset, uint32_t& timer_id) = 0;

  virtual std::string URIEncode(const std::string& value) = 0;

  virtual std::unique_ptr<ads::AdsURLLoader> LoadURL(
      const std::string& url, const std::vector<std::string>& headers,
      const std::string& content, const std::string& contentType,
      const ads::URL_METHOD& method, ads::CallbackHandler* handler) = 0;
};

}  // namespace ads
