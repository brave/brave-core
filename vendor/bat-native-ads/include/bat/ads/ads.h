/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "bat/ads/ad_content_action_types.h"
#include "bat/ads/ads_aliases.h"
#include "bat/ads/ads_history_filter_types.h"
#include "bat/ads/ads_history_sort_types.h"
#include "bat/ads/category_content_action_types.h"
#include "bat/ads/export.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

class AdsClient;
struct AdsHistoryInfo;
struct AdNotificationInfo;

// |g_environment| indicates that URL requests should use production, staging or
// development servers but can be overridden via command-line arguments
extern mojom::Environment g_environment;

// Returns the reference to the hardware |manufacturer| and |model|
mojom::SysInfo& SysInfo();

// Returns the reference to the build channel
mojom::BuildChannel& BuildChannel();

// |g_is_debug| indicates that the next catalog download should be reduced from
// ~1 hour to ~25 seconds. This value should be set to false on production
// builds and true on debug builds but can be overridden via command-line
// arguments
extern bool g_is_debug;

// Catalog schema resource id
extern const char g_catalog_schema_resource_id[];

// Returns true if the locale is supported otherwise returns false
bool IsSupportedLocale(const std::string& locale);

class ADS_EXPORT Ads {
 public:
  Ads() = default;
  virtual ~Ads() = default;

  static Ads* CreateInstance(AdsClient* ads_client);

  // Should be called to initialize ads when launching the browser or when ads
  // is enabled by a user. The callback takes one argument - |bool| should be
  // set to |true| if successful otherwise should be set to |false|
  virtual void Initialize(InitializeCallback callback) = 0;

  // Should be called to shutdown ads when a user disables ads. The callback
  // takes one argument - |bool| should be set to |true| if successful
  // otherwise should be set to |false|
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
      const mojom::AdNotificationEventType event_type) = 0;

  // Should be called to get an eligible new tab page ad
  virtual void GetNewTabPageAd(GetNewTabPageAdCallback callback) = 0;

  // Should be called when a user views or clicks a new tab page ad
  virtual void OnNewTabPageAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const mojom::NewTabPageAdEventType event_type) = 0;

  // Should be called when a user views or clicks a promoted content ad
  virtual void OnPromotedContentAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const mojom::PromotedContentAdEventType event_type) = 0;

  // Should be called to get an eligible inline content ad for the specified
  // size
  virtual void GetInlineContentAd(const std::string& dimensions,
                                  GetInlineContentAdCallback callback) = 0;

  // Should be called when a user views or clicks an inline content ad
  virtual void OnInlineContentAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const mojom::InlineContentAdEventType event_type) = 0;

  // When a user views or clicks an |ad_mojom| search result ad, we should
  // trigger an |event_type| event. Should only be called again after
  // |TriggerSearchResultAdEventCallback|
  virtual void TriggerSearchResultAdEvent(
      mojom::SearchResultAdPtr ad_mojom,
      const mojom::SearchResultAdEventType event_type,
      TriggerSearchResultAdEventCallback callback) = 0;

  // Purge orphaned ad events for the specified |ad_type|
  virtual void PurgeOrphanedAdEventsForType(const mojom::AdType ad_type) = 0;

  // Should be called to remove all cached history. The callback takes one
  // argument - |bool| should be set to |true| if successful otherwise should be
  // set to |false|
  virtual void RemoveAllHistory(RemoveAllHistoryCallback callback) = 0;

  // Should be called to get history for a specified date range. Returns
  // |AdsHistoryInfo|
  virtual AdsHistoryInfo GetHistory(const AdsHistoryFilterType filter_type,
                                    const AdsHistorySortType sort_type,
                                    const double from_timestamp,
                                    const double to_timestamp) = 0;

  // Should be called to get the statement of accounts. The callback takes one
  // argument - |StatementInfo| which contains next payment date, ads received
  // this month, earnings this month, earnings last month, cleared transactions
  // and uncleared transactions
  virtual void GetStatementOfAccounts(
      GetStatementOfAccountsCallback callback) = 0;

  // Should be called to get ad diagnostics for rewards internals page.
  virtual void GetAdDiagnostics(GetAdDiagnosticsCallback callback) = 0;

  // Should be called to indicate interest in the specified ad. This is a
  // toggle, so calling it again returns the setting to the neutral state
  virtual AdContentLikeActionType ToggleAdThumbUp(const std::string& json) = 0;

  // Should be called to indicate a lack of interest in the specified ad. This
  // is a toggle, so calling it again returns the setting to the neutral state
  virtual AdContentLikeActionType ToggleAdThumbDown(
      const std::string& json) = 0;

  // Should be called to opt-in to the specified ad category. This is a toggle,
  // so calling it again neutralizes the ad category. Returns
  // |CategoryContentOptActionType| with the current status
  virtual CategoryContentOptActionType ToggleAdOptIn(
      const std::string& category,
      const CategoryContentOptActionType& action) = 0;

  // Should be called to opt-out of the specified ad category. This is a toggle,
  // so calling it again neutralizes the ad category. Returns
  // |CategoryContentOptActionType| with the current status
  virtual CategoryContentOptActionType ToggleAdOptOut(
      const std::string& category,
      const CategoryContentOptActionType& action) = 0;

  // Should be called to save an ad for later viewing. This is a toggle, so
  // calling it again removes the ad from the saved list. Returns true if the ad
  // was saved otherwise should return false
  virtual bool ToggleSavedAd(const std::string& json) = 0;

  // Should be called to flag an ad as inappropriate. This is a toggle, so
  // calling it again unflags the ad. Returns true if the ad was flagged
  // otherwise returns false
  virtual bool ToggleFlaggedAd(const std::string& json) = 0;

 private:
  Ads(const Ads&) = delete;
  Ads& operator=(const Ads&) = delete;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_H_
