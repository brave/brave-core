/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/browser/ads_service_observer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class GURL;

namespace brave_ads {

class AdsService : public KeyedService {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void InitNotificationHelper() = 0;
    virtual bool CanShowSystemNotificationsWhileBrowserIsBackgrounded() = 0;
    virtual bool DoesSupportSystemNotifications() = 0;
    virtual bool CanShowNotifications() = 0;
    virtual bool ShowOnboardingNotification() = 0;
    virtual void ShowScheduledCaptcha(const std::string& payment_id,
                                      const std::string& captcha_id) = 0;
    virtual void ClearScheduledCaptcha() = 0;
    virtual void SnoozeScheduledCaptcha() = 0;
    virtual void ShowNotificationAd(const std::string& id,
                                    const std::u16string& title,
                                    const std::u16string& body,
                                    bool is_custom) = 0;
    virtual void CloseNotificationAd(const std::string& id, bool is_custom) = 0;
    virtual void OpenNewTabWithUrl(const GURL& url) = 0;
    virtual bool IsFullScreenMode() = 0;

    virtual base::Value::Dict GetVirtualPrefs() = 0;
  };

  explicit AdsService(std::unique_ptr<Delegate> delegate);

  AdsService(const AdsService&) = delete;
  AdsService& operator=(const AdsService&) = delete;

  AdsService(AdsService&&) noexcept = delete;
  AdsService& operator=(AdsService&&) noexcept = delete;

  ~AdsService() override;

  AdsService::Delegate* delegate() { return delegate_.get(); }

  virtual void AddObserver(AdsServiceObserver* observer) = 0;
  virtual void RemoveObserver(AdsServiceObserver* observer) = 0;

  // Returns true if a browser upgrade is required to serve ads.
  virtual bool IsBrowserUpgradeRequiredToServeAds() const = 0;

  // Returns the maximum number of notification ads that can be served per hour.
  virtual int64_t GetMaximumNotificationAdsPerHour() const = 0;

  // Called when a notification ad with `placement_id` is shown.
  virtual void OnNotificationAdShown(const std::string& placement_id) = 0;

  // Called when a notification ad with `placement_id` is closed. `by_user` is
  // `true` if the user closed the notification otherwise `false`.
  virtual void OnNotificationAdClosed(const std::string& placement_id,
                                      bool by_user) = 0;

  // Called when a notification ad with `placement_id` is clicked.
  virtual void OnNotificationAdClicked(const std::string& placement_id) = 0;

  // Called to clear ads data.
  virtual void ClearData(base::OnceClosure callback) = 0;

  // Called to add an ads observer.
  virtual void AddBatAdsObserver(
      mojo::PendingRemote<bat_ads::mojom::BatAdsObserver>
          bat_ads_observer_pending_remote) = 0;

  // Called to get diagnostics to help identify issues. The callback takes one
  // argument - `base::Value::List` containing info of the obtained diagnostics.
  virtual void GetDiagnostics(GetDiagnosticsCallback callback) = 0;

  // Called to get the statement of accounts. The callback takes one argument -
  // `mojom::StatementInfo` containing info of the obtained statement of
  // accounts.
  virtual void GetStatementOfAccounts(
      GetStatementOfAccountsCallback callback) = 0;

  // Called to serve an inline content ad for the specified `dimensions`. The
  // callback takes two arguments - `std::string` containing the dimensions and
  // `InlineContentAdInfo` containing the info for the ad.
  virtual void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) = 0;

  // Called when a user views or interacts with an inline content ad to trigger
  // a `mojom_ad_event_type` event for the specified `placement_id` and
  // `creative_instance_id`. `placement_id` should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same `placement_id`
  // generated for the viewed impression event should be used for all other
  // events for the same ad placement. The callback takes one argument - `bool`
  // is set to `true` if successful otherwise `false`. Must be called before the
  // `mojom::InlineContentAdEventType::target_url` landing page is opened.
  virtual void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) = 0;

  // Called to prefetch a new tab page ad.
  virtual void PrefetchNewTabPageAd() = 0;

  // Called to get the prefetched new tab page ad for display.
  virtual std::optional<NewTabPageAdInfo>
  MaybeGetPrefetchedNewTabPageAdForDisplay() = 0;

  // Called when failing to prefetch a new tab page ad for the specified
  // `placement_id` and `creative_instance_id`.
  virtual void OnFailedToPrefetchNewTabPageAd(
      const std::string& placement_id,
      const std::string& creative_instance_id) = 0;

  // Called when a user views or interacts with a new tab page ad to trigger a
  // `mojom_ad_event_type` event for the specified `placement_id` and
  // `creative_instance_id`. `placement_id` should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same `placement_id`
  // generated for the viewed impression event should be used for all other
  // events for the same ad placement. The callback takes one argument - `bool`
  // is set to `true if successful otherwise `false`. Must be called before the
  // `mojom::NewTabPageAdEventType::target_url` landing page is opened.
  virtual void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) = 0;

  // Called when a user views or interacts with a promoted content ad to trigger
  // a `mojom_ad_event_type` event for the specified `placement_id` and
  // `creative_instance_id`. `placement_id` should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same `placement_id`
  // generated for the viewed impression event should be used for all other
  // events for the same ad placement. The callback takes one argument - `bool`
  // is set to `true` if successful otherwise `false`. Must be called before the
  // `mojom::PromotedContentAdEventType::target_url` landing page is opened.
  virtual void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) = 0;

  // Called to get the search result ad specified by `placement_id`. The
  // callback takes one argument - `mojom::CreativeSearchResultAdInfoPtr`
  // containing the info of the search result ad.
  virtual void MaybeGetSearchResultAd(
      const std::string& placement_id,
      MaybeGetSearchResultAdCallback callback) = 0;

  // Called when a user views or interacts with a search result ad to trigger a
  // `mojom_ad_event_type` event for the ad specified in `mojom_creative_ad`.
  // The callback takes one argument - `bool` is set to `true` if successful
  // otherwise `false`. Must be called before the
  // `mojom::CreativeSearchResultAdInfo::target_url` landing page is opened.
  virtual void TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) = 0;

  // Called to purge orphaned served ad events for the specified `mojom_ad_type`
  // before calling `MaybeServe*Ad`. The callback takes one argument - `bool` is
  // set to `true` if successful otherwise `false`.
  virtual void PurgeOrphanedAdEventsForType(
      mojom::AdType mojom_ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) = 0;

  // Called to get ad history for the given date range in descending order. The
  // callback takes one argument - `base::Value::List` containing info of the
  // obtained ad history.
  virtual void GetAdHistory(base::Time from_time,
                            base::Time to_time,
                            GetAdHistoryForUICallback callback) = 0;

  // Called to like an ad. This is a toggle, so calling it again returns the
  // setting to the neutral state. The callback takes one argument - `bool` is
  // set to `true` if successful otherwise `false`.
  virtual void ToggleLikeAd(mojom::ReactionInfoPtr mojom_reaction,
                            ToggleReactionCallback callback) = 0;

  // Called to dislike an ad. This is a toggle, so calling it again returns the
  // setting to the neutral state. The callback takes one argument - `bool` is
  // set to `true` if successful otherwise `false`.
  virtual void ToggleDislikeAd(mojom::ReactionInfoPtr mojom_reaction,
                               ToggleReactionCallback callback) = 0;

  // Called to like a category. This is a toggle, so calling it again returns
  // the setting to the neutral state. The callback takes one argument - `bool`
  // is set to `true` if successful otherwise `false`.
  virtual void ToggleLikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                                 ToggleReactionCallback callback) = 0;

  // Called to dislike a category. This is a toggle, so calling it again
  // returns the setting to the neutral state. The callback takes one argument -
  // `bool` is set to `true` if successful otherwise `false`.
  virtual void ToggleDislikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                                    ToggleReactionCallback callback) = 0;

  // Called to save an ad for later viewing. This is a toggle, so calling it
  // again removes the ad from the saved list. The callback takes one argument -
  // `bool` is set to `true` if successful otherwise `false`.
  virtual void ToggleSaveAd(mojom::ReactionInfoPtr reactimojom_reactionon,
                            ToggleReactionCallback callback) = 0;

  // Called to mark an ad as inappropriate. This is a toggle, so calling it
  // again unmarks the ad. The callback takes one argument - `bool` is
  // set to `true` if successful otherwise `false`.
  virtual void ToggleMarkAdAsInappropriate(
      mojom::ReactionInfoPtr mojom_reaction,
      ToggleReactionCallback callback) = 0;

  // Called when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `text` containing the page content as text.
  virtual void NotifyTabTextContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& text) = 0;

  // Called when the page for `tab_id` has loaded and the content is available
  // for analysis. `redirect_chain` containing a list of redirect URLs that
  // occurred on the way to the current page. The current page is the last one
  // in the list (so even when there's no redirect, there should be one entry in
  // the list). `html` containing the page content as HTML.
  virtual void NotifyTabHtmlContentDidChange(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      const std::string& html) = 0;

  // Called when media starts playing on a browser tab for the specified
  // `tab_id`.
  virtual void NotifyTabDidStartPlayingMedia(int32_t tab_id) = 0;

  // Called when media stops playing on a browser tab for the specified
  // `tab_id`.
  virtual void NotifyTabDidStopPlayingMedia(int32_t tab_id) = 0;

  // Called when a browser tab is updated with the specified `redirect_chain`
  // containing a list of redirect URLs that occurred on the way to the current
  // page. The current page is the last one in the list (so even when there's no
  // redirect, there should be one entry in the list). `is_restoring` should be
  // set to `true` if the page is restoring otherwise should be set to `false`.
  // `is_visible` should be set to `true` if `tab_id` refers to the currently
  // visible tab otherwise should be set to `false`.
  virtual void NotifyTabDidChange(int32_t tab_id,
                                  const std::vector<GURL>& redirect_chain,
                                  bool is_new_navigation,
                                  bool is_restoring,
                                  bool is_visible) = 0;

  // Called when a browser tab has loaded. `http_status_code` should be set to
  // the HTTP status code.
  virtual void NotifyTabDidLoad(int32_t tab_id, int http_status_code) = 0;

  // Called when a browser tab with the specified `tab_id` is closed.
  virtual void NotifyDidCloseTab(int32_t tab_id) = 0;

  // Called when a page navigation was initiated by a user gesture.
  // `page_transition_type` containing the page transition type, see enums for
  // `PageTransitionType`.
  virtual void NotifyUserGestureEventTriggered(
      int32_t page_transition_type) = 0;

  // Called when the browser did become active.
  virtual void NotifyBrowserDidBecomeActive() = 0;

  // Called when the browser did resign active.
  virtual void NotifyBrowserDidResignActive() = 0;

  // Called when the user solves an adaptive captcha.
  virtual void NotifyDidSolveAdaptiveCaptcha() = 0;

 protected:
  std::unique_ptr<Delegate> delegate_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
