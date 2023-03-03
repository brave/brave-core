/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "bat/ads/ad_content_action_types.h"
#include "bat/ads/ads_callback.h"
#include "bat/ads/category_content_action_types.h"
#include "bat/ads/export.h"
#include "bat/ads/history_filter_types.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/history_sort_types.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

class GURL;

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace ads {

class AdsClient;
struct NotificationAdInfo;

// Returns |true| if the locale is supported otherwise returns |false|.
bool IsSupportedLocale(const std::string& locale);

class ADS_EXPORT Ads {
 public:
  Ads() = default;

  Ads(const Ads& other) = delete;
  Ads& operator=(const Ads& other) = delete;

  Ads(Ads&& other) noexcept = delete;
  Ads& operator=(Ads&& other) noexcept = delete;

  virtual ~Ads() = default;

  static Ads* CreateInstance(AdsClient* ads_client);

  // Called to initialize ads. The callback takes one argument - |bool| is set
  // to |true| if successful otherwise |false|.
  virtual void Initialize(InitializeCallback callback) = 0;

  // Called to shutdown ads. The callback takes one argument - |bool| is set to
  // |true| if successful otherwise |false|.
  virtual void Shutdown(ShutdownCallback callback) = 0;

  // Called to get diagnostics to help identify issues. The callback takes one
  // argument - |base::Value::List| containing info of the obtained diagnostics.
  virtual void GetDiagnostics(GetDiagnosticsCallback callback) = 0;

  // Called when the user changes the locale of their operating system. This
  // call is not required if the operating system restarts the browser when
  // changing the locale. |locale| should be specified in either
  // <ISO-639-1>-<ISO-3166-1> or <ISO-639-1>_<ISO-3166-1> format.
  virtual void OnLocaleDidChange(const std::string& locale) = 0;

  // Called when a preference has changed for the specified |path|.
  virtual void OnPrefDidChange(const std::string& path) = 0;

  // Called when a resource component has been updated.
  virtual void OnDidUpdateResourceComponent(const std::string& id) = 0;

  // Called when the page for |tad_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |html| containing the page content as HTML.
  virtual void OnTabHtmlContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& html) = 0;

  // Called when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). |text| containing the page content as text.
  virtual void OnTabTextContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& text) = 0;

  // Called when a user has been idle for the threshold set in
  // |prefs::kIdleTimeThreshold|. NOTE: This should not be called on mobile
  // devices.
  virtual void OnUserDidBecomeIdle() = 0;

  // Called when a user is no longer idle. |idle_time| is the amount of time in
  // seconds that the user was idle. |screen_was_locked| should be |true| if the
  // screen was locked, otherwise |false|. NOTE: This should not be called on
  // mobile devices.
  virtual void OnUserDidBecomeActive(base::TimeDelta idle_time,
                                     bool screen_was_locked) = 0;

  // Called when a page navigation was initiated by a user gesture.
  // |page_transition_type| containing the page transition type, see enums for
  // |PageTransitionType|.
  virtual void TriggerUserGestureEvent(int32_t page_transition_type) = 0;

  // Called when the browser did enter the foreground.
  virtual void OnBrowserDidEnterForeground() = 0;

  // Called when the browser did enter the background.
  virtual void OnBrowserDidEnterBackground() = 0;

  // Called when media starts playing on a browser tab for the specified
  // |tab_id|.
  virtual void OnTabDidStartPlayingMedia(int32_t tab_id) = 0;

  // Called when media stops playing on a browser tab for the specified
  // |tab_id|.
  virtual void OnTabDidStopPlayingMedia(int32_t tab_id) = 0;

  // Called when a browser tab is updated with the specified |redirect_chain|
  // containing a list of redirect URLs that occurred on the way to the current
  // page. The current page is the last one in the list (so even when there's no
  // redirect, there should be one entry in the list). |is_active| is set to
  // |true| if |tab_id| refers to the currently active tab otherwise is set to
  // |false|. |is_browser_active| is set to |true| if the browser window is
  // active otherwise |false|. |is_incognito| is set to |true| if the tab is
  // incognito otherwise |false|.
  virtual void OnTabDidChange(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              bool is_active,
                              bool is_browser_active,
                              bool is_incognito) = 0;

  // Called when a browser tab with the specified |tab_id| was closed.
  virtual void OnDidCloseTab(int32_t tab_id) = 0;

  // Called when the user's Brave Rewards wallet has changed.
  virtual void OnRewardsWalletDidChange(const std::string& payment_id,
                                        const std::string& recovery_seed) = 0;

  // Called to get the statement of accounts. The callback takes one argument -
  // |mojom::StatementInfo| containing info of the obtained statement of
  // accounts.
  virtual void GetStatementOfAccounts(
      GetStatementOfAccountsCallback callback) = 0;

  // Should be called to serve an inline content ad for the specified
  // |dimensions|. The callback takes two arguments - |std::string| containing
  // the dimensions and |InlineContentAdInfo| containing the info for the ad.
  virtual void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) = 0;

  // Called when a user views or interacts with an inline content ad to trigger
  // an |event_type| event for the specified |placement_id| and
  // |creative_instance_id|. |placement_id| should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same |placement_id|
  // generated for the viewed event should be used for all other events for the
  // same ad placement.
  virtual void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType event_type) = 0;

  // Should be called to serve a new tab page ad. The callback takes one
  // argument - |NewTabPageAdInfo| containing the info for the ad.
  virtual void MaybeServeNewTabPageAd(
      MaybeServeNewTabPageAdCallback callback) = 0;

  // Called when a user views or interacts with a new tab page ad to trigger an
  // |event_type| event for the specified |placement_id| and
  // |creative_instance_id|. |placement_id| should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same |placement_id|
  // generated for the viewed event should be used for all other events for the
  // same ad placement.
  virtual void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdEventType event_type) = 0;

  // Called to get the notification ad specified by |placement_id|. Returns
  // |NotificationAdInfo| containing the info of the ad.
  virtual absl::optional<NotificationAdInfo> MaybeGetNotificationAd(
      const std::string& placement_id) = 0;

  // Called when a user views or interacts with a notification ad or the ad
  // notification times out to trigger an |event_type| event for the specified
  // |placement_id|. |placement_id| should be a 128-bit random GUID in the form
  // of version 4. See RFC 4122, section 4.4. The same |placement_id| generated
  // for the viewed event should be used for all other events for the same ad
  // placement.
  virtual void TriggerNotificationAdEvent(
      const std::string& placement_id,
      mojom::NotificationAdEventType event_type) = 0;

  // Called when a user views or interacts with a promoted content ad to trigger
  // an |event_type| event for the specified |placement_id| and
  // |creative_instance_id|. |placement_id| should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same |placement_id|
  // generated for the viewed event should be used for all other events for the
  // same ad placement.
  virtual void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType event_type) = 0;

  // Called when a user views or interacts with a search result ad to trigger an
  // |event_type| event for the ad specified in |ad_mojom|.
  virtual void TriggerSearchResultAdEvent(
      mojom::SearchResultAdInfoPtr ad_mojom,
      mojom::SearchResultAdEventType event_type) = 0;

  // Called to purge orphaned served ad events. NOTE: You should call before
  // triggering new ad events for the specified |ad_type|.
  virtual void PurgeOrphanedAdEventsForType(
      mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) = 0;

  // Called to get history filtered by |filter_type| and sorted by |sort_type|
  // between |from_time| and |to_time| date range. Returns |HistoryItemList|
  // containing info of the obtained history.
  virtual HistoryItemList GetHistory(HistoryFilterType filter_type,
                                     HistorySortType sort_type,
                                     base::Time from_time,
                                     base::Time to_time) = 0;

  // Called to remove all history. The callback takes one argument - |bool| is
  // set to |true| if successful otherwise |false|.
  virtual void RemoveAllHistory(RemoveAllHistoryCallback callback) = 0;

  // Called to like an advertiser. This is a toggle, so calling it again returns
  // the setting to the neutral state. Returns |AdContentLikeActionType|
  // containing the current state.
  virtual AdContentLikeActionType ToggleAdThumbUp(base::Value::Dict value) = 0;

  // Called to dislike an advertiser. This is a toggle, so calling it again
  // returns the setting to the neutral state. Returns |AdContentLikeActionType|
  // containing the current state.
  virtual AdContentLikeActionType ToggleAdThumbDown(
      base::Value::Dict value) = 0;

  // Called to no longer receive ads for the specified category. This is a
  // toggle, so calling it again returns the setting to the neutral state.
  // Returns |CategoryContentOptActionType| containing the current state.
  virtual CategoryContentOptActionType ToggleAdOptIn(
      const std::string& category,
      const CategoryContentOptActionType& action_type) = 0;

  // Called to receive ads for the specified category. This is a toggle, so
  // calling it again returns the setting to the neutral state. Returns
  // |CategoryContentOptActionType| containing the current state.
  virtual CategoryContentOptActionType ToggleAdOptOut(
      const std::string& category,
      const CategoryContentOptActionType& action_type) = 0;

  // Called to save an ad for later viewing. This is a toggle, so calling it
  // again removes the ad from the saved list. Returns |true| if the ad was
  // saved otherwise |false|.
  virtual bool ToggleSavedAd(base::Value::Dict value) = 0;

  // Called to mark an ad as inappropriate. This is a toggle, so calling it
  // again unmarks the ad. Returns |true| if the ad was marked otherwise
  // |false|.
  virtual bool ToggleFlaggedAd(base::Value::Dict value) = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_H_
