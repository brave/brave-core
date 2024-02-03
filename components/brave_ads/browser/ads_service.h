/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class GURL;

namespace brave_ads {

class AdsService : public KeyedService {
 public:
  AdsService();

  AdsService(const AdsService&) = delete;
  AdsService& operator=(const AdsService&) = delete;

  AdsService(AdsService&&) noexcept = delete;
  AdsService& operator=(AdsService&&) noexcept = delete;

  ~AdsService() override;

  // Returns the maximum number of notification ads that can be served per hour.
  virtual int64_t GetMaximumNotificationAdsPerHour() const = 0;

  // Called to show a notification indicating that a scheduled captcha with the
  // given `captcha_id` must be solved for the given `payment_id` before the
  // user can continue to served ads.
  virtual void ShowScheduledCaptcha(const std::string& payment_id,
                                    const std::string& captcha_id) = 0;

  // Called to snooze the scheduled captcha, if any.
  virtual void SnoozeScheduledCaptcha() = 0;

  // Called when a notification ad with `placement_id` is shown.
  virtual void OnNotificationAdShown(const std::string& placement_id) = 0;

  // Called when a notification ad with `placement_id` is closed. `by_user` is
  // `true` if the user closed the notification otherwise `false`.
  virtual void OnNotificationAdClosed(const std::string& placement_id,
                                      bool by_user) = 0;

  // Called when a notification ad with `placement_id` is clicked.
  virtual void OnNotificationAdClicked(const std::string& placement_id) = 0;

  // Called to add an ads observer.
  virtual void AddBatAdsObserver(
      mojo::PendingRemote<bat_ads::mojom::BatAdsObserver> observer) = 0;

  // Called to get diagnostics to help identify issues. The callback takes one
  // argument - `base::Value::List` containing info of the obtained diagnostics.
  virtual void GetDiagnostics(GetDiagnosticsCallback callback) = 0;

  // Called to get the statement of accounts. The callback takes one argument -
  // `mojom::StatementInfo` containing info of the obtained statement of
  // accounts.
  virtual void GetStatementOfAccounts(
      GetStatementOfAccountsCallback callback) = 0;

  // Returns true if a browser upgrade is required to serve ads.
  virtual bool IsBrowserUpgradeRequiredToServeAds() const = 0;

  // Should be called to serve an inline content ad for the specified
  // `dimensions`. The callback takes two arguments - `std::string` containing
  // the dimensions and `base::Value::Dict` containing the info for the ad.
  virtual void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdAsDictCallback callback) = 0;

  // Called when a user views or interacts with an inline content ad to trigger
  // an `event_type` event for the specified `placement_id` and
  // `creative_instance_id`. `placement_id` should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same `placement_id`
  // generated for the viewed event should be used for all other events for the
  // same ad placement. The callback takes one argument - `bool` is set to
  // `true` if successful otherwise `false`.
  virtual void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType event_type,
      TriggerAdEventCallback callback) = 0;

  // Called to prefetch a new tab page ad.
  virtual void PrefetchNewTabPageAd() = 0;

  // Called to get the prefetched new tab page ad for display.
  virtual std::optional<NewTabPageAdInfo>
  GetPrefetchedNewTabPageAdForDisplay() = 0;

  // Called when failing to prefetch a new tab page ad for the specified
  // `placement_id` and `creative_instance_id`.
  virtual void OnFailedToPrefetchNewTabPageAd(
      const std::string& placement_id,
      const std::string& creative_instance_id) = 0;

  // Called when a user views or interacts with a new tab page ad to trigger an
  // `event_type` event for the specified `placement_id` and
  // `creative_instance_id`. `placement_id` should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same `placement_id`
  // generated for the viewed event should be used for all other events for the
  // same ad placement. The callback takes one argument - `bool` is set to
  // `true if successful otherwise `false`.
  virtual void TriggerNewTabPageAdEvent(const std::string& placement_id,
                                        const std::string& creative_instance_id,
                                        mojom::NewTabPageAdEventType event_type,
                                        TriggerAdEventCallback callback) = 0;

  // Called when a user views or interacts with a promoted content ad to trigger
  // an `event_type` event for the specified `placement_id` and
  // `creative_instance_id`. `placement_id` should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same `placement_id`
  // generated for the viewed event should be used for all other events for the
  // same ad placement. The callback takes one argument - `bool` is set to
  // `true` if successful otherwise `false`.
  virtual void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType event_type,
      TriggerAdEventCallback callback) = 0;

  // Called when a user views or interacts with a search result ad to trigger an
  // `event_type` event for the ad specified in `ad_mojom`. The callback takes
  // one argument - `bool` is set to `true` if successful otherwise `false`.
  virtual void TriggerSearchResultAdEvent(
      mojom::SearchResultAdInfoPtr ad_mojom,
      mojom::SearchResultAdEventType event_type,
      TriggerAdEventCallback callback) = 0;

  // Called to purge orphaned served ad events for the specified `ad_type`
  // before calling `MaybeServe*Ad()`. The callback takes one argument - `bool`
  // is set to `true` if successful otherwise `false`.
  virtual void PurgeOrphanedAdEventsForType(
      mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) = 0;

  // Called to get history between `from_time` and `to_time` date range. The
  // callback takes one argument - `base::Value::List` containing info of the
  // obtained history.
  virtual void GetHistory(base::Time from_time,
                          base::Time to_time,
                          GetHistoryCallback callback) = 0;

  // Called to like an advertiser. This is a toggle, so calling it again returns
  // the setting to the neutral state. The callback takes one argument -
  // `base::Value::Dict` containing the current state.
  virtual void ToggleLikeAd(base::Value::Dict value,
                            ToggleLikeAdCallback callback) = 0;

  // Called to dislike an advertiser. This is a toggle, so calling it again
  // returns the setting to the neutral state. The callback takes one argument -
  // `base::Value::Dict` containing the current state.
  virtual void ToggleDislikeAd(base::Value::Dict value,
                               ToggleDislikeAdCallback callback) = 0;

  // Called to like a category. This is a toggle, so calling it again returns
  // the setting to the neutral state. The callback takes one argument -
  // `base::Value::Dict` containing the current state.
  virtual void ToggleLikeCategory(base::Value::Dict value,
                                  ToggleLikeCategoryCallback callback) = 0;

  // Called to dislike a category. This is a toggle, so calling it again
  // returns the setting to the neutral state. The callback takes one argument -
  // `base::Value::Dict` containing the current state.
  virtual void ToggleDislikeCategory(
      base::Value::Dict value,
      ToggleDislikeCategoryCallback callback) = 0;

  // Called to save an ad for later viewing. This is a toggle, so calling it
  // again removes the ad from the saved list. The callback takes one argument -
  // `base::Value::Dict` containing the current state.
  virtual void ToggleSaveAd(base::Value::Dict value,
                            ToggleSaveAdCallback callback) = 0;

  // Called to mark an ad as inappropriate. This is a toggle, so calling it
  // again unmarks the ad. The callback takes one argument - `base::Value::Dict`
  // containing the current state.
  virtual void ToggleMarkAdAsInappropriate(
      base::Value::Dict value,
      ToggleMarkAdAsInappropriateCallback callback) = 0;

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `text` containing the page content as text.
  virtual void NotifyTabTextContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& text) = 0;

  // Invoked when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `html` containing the page content as HTML.
  virtual void NotifyTabHtmlContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& html) = 0;

  // Invoked when media starts playing on a browser tab for the specified
  // `tab_id`.
  virtual void NotifyTabDidStartPlayingMedia(int32_t tab_id) = 0;

  // Called when media stops playing on a browser tab for the specified
  // `tab_id`.
  virtual void NotifyTabDidStopPlayingMedia(int32_t tab_id) = 0;

  // Invoked when a browser tab is updated with the specified `redirect_chain`
  // containing a list of redirect URLs that occurred on the way to the current
  // page. The current page is the last one in the list (so even when there's no
  // redirect, there should be one entry in the list). `is_visible` should be
  // set to `true` if `tab_id` refers to the currently visible tab otherwise
  // should be set to `false`.
  virtual void NotifyTabDidChange(int32_t tab_id,
                                  const std::vector<GURL>& redirect_chain,
                                  bool is_visible) = 0;

  // Invoked when a browser tab with the specified `tab_id` is closed.
  virtual void NotifyDidCloseTab(int32_t tab_id) = 0;

  // Called when a page navigation was initiated by a user gesture.
  // `page_transition_type` containing the page transition type, see enums for
  // `PageTransitionType`.
  virtual void NotifyUserGestureEventTriggered(
      int32_t page_transition_type) = 0;

  // Invoked when the browser did become active.
  virtual void NotifyBrowserDidBecomeActive() = 0;

  // Invoked when the browser did resign active.
  virtual void NotifyBrowserDidResignActive() = 0;

  // Invoked when the user solves an adaptive captch.
  virtual void NotifyDidSolveAdaptiveCaptcha() = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
