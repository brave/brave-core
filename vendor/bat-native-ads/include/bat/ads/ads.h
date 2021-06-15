/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "bat/ads/ad_content_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/category_content_info.h"
#include "bat/ads/export.h"
#include "bat/ads/mojom.h"
#include "bat/ads/promoted_content_ad_info.h"
#include "bat/ads/result.h"
#include "bat/ads/statement_info.h"

namespace ads {

using InitializeCallback = std::function<void(const Result)>;
using ShutdownCallback = std::function<void(const Result)>;

using RemoveAllHistoryCallback = std::function<void(const Result)>;

using GetAccountStatementCallback =
    std::function<void(const bool, const StatementInfo&)>;

// |g_environment| indicates that URL requests should use production, staging or
// development servers but can be overridden via command-line arguments
extern Environment g_environment;

// |g_sys_info| contains the hardware |manufacturer| and |model|
extern SysInfo g_sys_info;

// |g_build_channel| indicates the build channel
extern BuildChannel g_build_channel;

// |g_is_debug| indicates that the next catalog download should be reduced from
// ~1 hour to ~25 seconds. This value should be set to false on production
// builds and true on debug builds but can be overridden via command-line
// arguments
extern bool g_is_debug;

// Catalog schema resource id
extern const char g_catalog_schema_resource_id[];

// Returns true if the locale is supported otherwise returns false
bool IsSupportedLocale(const std::string& locale);

// Returns true if the locale is newly supported otherwise returns false
bool IsNewlySupportedLocale(const std::string& locale,
                            const int last_schema_version);

class ADS_EXPORT Ads {
 public:
  Ads() = default;
  virtual ~Ads() = default;

  static Ads* CreateInstance(AdsClient* ads_client);

  // Should be called to initialize ads when launching the browser or when ads
  // is enabled by a user. The callback takes one argument - |Result| should be
  // set to |SUCCESS| if successful otherwise should be set to |FAILED|
  virtual void Initialize(InitializeCallback callback) = 0;

  // Should be called to shutdown ads when a user disables ads. The callback
  // takes one argument - |Result| should be set to |SUCCESS| if successful
  // otherwise should be set to |FAILED|
  virtual void Shutdown(ShutdownCallback callback) = 0;

  // Should be called when the user changes the locale of their operating
  // system. This call is not required if the operating system restarts the
  // browser when changing the locale. |locale| should be specified in either
  // <ISO-639-1>-<ISO-3166-1> or <ISO-639-1>_<ISO-3166-1> format
  virtual void ChangeLocale(const std::string& locale) = 0;

  // Should be called when a pref changes. |path| contains the pref path
  virtual void OnPrefChanged(const std::string& path) = 0;

  // Should be called when a page has loaded and the content is available for
  // analysis. |redirect_chain| contains the chain of redirects, including
  // client-side redirect and the current URL. |html| will contain the page
  // content as HTML
  virtual void OnHtmlLoaded(const int32_t tab_id,
                            const std::vector<std::string>& redirect_chain,
                            const std::string& html) = 0;

  // Should be called when a page has loaded and the content is available for
  // analysis. |redirect_chain| contains the chain of redirects, including
  // client-side redirect and the current URL. |text| will contain the page
  // content as text
  virtual void OnTextLoaded(const int32_t tab_id,
                            const std::vector<std::string>& redirect_chain,
                            const std::string& text) = 0;

  // Should be called when the navigation was initiated by a user gesture.
  // |page_transition_type| contains the page transition type
  virtual void OnUserGesture(const int32_t page_transition_type) = 0;

  // Should be called when a user is no longer idle. |idle_time| returns the
  // idle time in seconds. |was_locked| returns true if the screen is locked,
  // otherwise should be set to false. This should not be called on mobile
  // devices
  virtual void OnUnIdle(const int idle_time, const bool was_locked) = 0;

  // Should be called when a user is idle for the threshold set in
  // |prefs::kIdleTimeThreshold|. This should not be called on mobile devices
  virtual void OnIdle() = 0;

  // Should be called when the browser becomes active
  virtual void OnForeground() = 0;

  // Should be called when the browser enters the background
  virtual void OnBackground() = 0;

  // Should be called when media starts playing on a browser tab
  virtual void OnMediaPlaying(const int32_t tab_id) = 0;

  // Should be called when media stops playing on a browser tab
  virtual void OnMediaStopped(const int32_t tab_id) = 0;

  // Should be called when a browser tab is updated. |is_active| should be set
  // to true if |tab_id| refers to the currently active tab otherwise should be
  // set to false. |is_browser_active| should be set to true if the current
  // browser window is active otherwise should be set to false. |is_incognito|
  // should be set to true if the tab is private otherwise should be set to
  // false
  virtual void OnTabUpdated(const int32_t tab_id,
                            const std::string& url,
                            const bool is_active,
                            const bool is_browser_active,
                            const bool is_incognito) = 0;

  // Should be called when a browser tab is closed
  virtual void OnTabClosed(const int32_t tab_id) = 0;

  // Should be called when the users wallet has been updated
  virtual void OnWalletUpdated(const std::string& payment_id,
                               const std::string& seed) = 0;

  // Should be called when a resource component has been updated by
  // |brave_ads::ResourceComponent|
  virtual void OnResourceComponentUpdated(const std::string& id) = 0;

  // Should be called to get the ad notification specified by |uuid|. Returns
  // true if the ad notification exists otherwise returns false.
  // |ad_notification| contains the ad notification for uuid
  virtual bool GetAdNotification(const std::string& uuid,
                                 AdNotificationInfo* ad_notification) = 0;

  // Should be called when a user views, clicks or dismisses an ad notification
  // or an ad notification times out
  virtual void OnAdNotificationEvent(
      const std::string& uuid,
      const AdNotificationEventType event_type) = 0;

  // Should be called when a user views or clicks a new tab page ad
  virtual void OnNewTabPageAdEvent(const std::string& uuid,
                                   const std::string& creative_instance_id,
                                   const NewTabPageAdEventType event_type) = 0;

  // Should be called when a user views or clicks a promoted content ad
  virtual void OnPromotedContentAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const PromotedContentAdEventType event_type) = 0;

  // Should be called to remove all cached history. The callback takes one
  // argument - |Result| should be set to |SUCCESS| if successful otherwise
  // should be set to |FAILED|
  virtual void RemoveAllHistory(RemoveAllHistoryCallback callback) = 0;

  // Should be called to reconcile ad rewards with the server, i.e. after an
  // ad grant is claimed
  virtual void ReconcileAdRewards() = 0;

  // Should be called to get ads history for a specified date range. Returns
  // |AdsHistoryInfo|
  virtual AdsHistoryInfo GetAdsHistory(
      const AdsHistoryInfo::FilterType filter_type,
      const AdsHistoryInfo::SortType sort_type,
      const uint64_t from_timestamp,
      const uint64_t to_timestamp) = 0;

  // Should be called to get the statement of accounts. The callback takes one
  // argument - |StatementInfo| which contains estimated pending rewards, next
  // payment date, ads received this month, pending rewards, cleared
  // transactions and uncleared transactions
  virtual void GetAccountStatement(GetAccountStatementCallback callback) = 0;

  // Should be called to indicate interest in the specified ad. This is a
  // toggle, so calling it again returns the setting to the neutral state
  virtual AdContentInfo::LikeAction ToggleAdThumbUp(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContentInfo::LikeAction& action) = 0;

  // Should be called to indicate a lack of interest in the specified ad. This
  // is a toggle, so calling it again returns the setting to the neutral state
  virtual AdContentInfo::LikeAction ToggleAdThumbDown(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContentInfo::LikeAction& action) = 0;

  // Should be called to opt-in to the specified ad category. This is a toggle,
  // so calling it again neutralizes the ad category. Returns |OptAction" with
  // the current status
  virtual CategoryContentInfo::OptAction ToggleAdOptInAction(
      const std::string& category,
      const CategoryContentInfo::OptAction& action) = 0;

  // Should be called to opt-out of the specified ad category. This is a toggle,
  // so calling it again neutralizes the ad category. Returns |OptAction" with
  // the current status
  virtual CategoryContentInfo::OptAction ToggleAdOptOutAction(
      const std::string& category,
      const CategoryContentInfo::OptAction& action) = 0;

  // Should be called to save an ad for later viewing. This is a toggle, so
  // calling it again removes the ad from the saved list. Returns true if the ad
  // was saved otherwise should return false
  virtual bool ToggleSaveAd(const std::string& creative_instance_id,
                            const std::string& creative_set_id,
                            const bool saved) = 0;

  // Should be called to flag an ad as inappropriate. This is a toggle, so
  // calling it again unflags the ad. Returns true if the ad was flagged
  // otherwise returns false
  virtual bool ToggleFlagAd(const std::string& creative_instance_id,
                            const std::string& creative_set_id,
                            const bool flagged) = 0;

 private:
  // Not copyable, not assignable
  Ads(const Ads&) = delete;
  Ads& operator=(const Ads&) = delete;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_H_
