/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/time/time.h"
#include "brave/components/brave_adaptive_captcha/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/browser/ads_service_observer.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/new_tab_page_ad_info.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace brave_ads {

class AdsService : public KeyedService {
 public:
  AdsService();
  AdsService(const AdsService&) = delete;
  AdsService& operator=(const AdsService&) = delete;
  ~AdsService() override;

  void AddObserver(AdsServiceObserver* observer);
  void RemoveObserver(AdsServiceObserver* observer);

  // static
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns |true| if the user's locale supports ads.
  virtual bool IsSupportedLocale() const = 0;

  // Returns |true| if ads are enabled.
  virtual bool IsEnabled() const = 0;

  // Called to enable or disable ads.
  virtual void SetEnabled(const bool is_enabled) = 0;

  // Returns the number of notification ads that can be served per hour.
  virtual int64_t GetNotificationAdsPerHour() const = 0;

  // Called to set the number of notification ads that can be served per hour.
  virtual void SetNotificationAdsPerHour(const int64_t ads_per_hour) = 0;

  // Called to allow or disallow conversion tracking.
  virtual void SetAllowConversionTracking(const bool should_allow) = 0;

  // Returns |true| if subdivision targeting is supported.
  virtual bool ShouldAllowSubdivisionTargeting() const = 0;

  // Returns the subdivision targeting code. See
  // https://en.wikipedia.org/wiki/ISO_3166-2.
  virtual std::string GetSubdivisionTargetingCode() const = 0;

  // Called to set the subdivision targeting code. See
  // https://en.wikipedia.org/wiki/ISO_3166-2.
  virtual void SetSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) = 0;

  // Returns the auto detected subdivision targeting code. See
  // https://en.wikipedia.org/wiki/ISO_3166-2.
  virtual std::string GetAutoDetectedSubdivisionTargetingCode() const = 0;

  // Called to set the auto detected subdivision targeting code. See
  // https://en.wikipedia.org/wiki/ISO_3166-2.
  virtual void SetAutoDetectedSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) = 0;

  // Called if a browser upgrade is required to serve ads.
  virtual bool NeedsBrowserUpgradeToServeAds() const = 0;

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  // Called to show a notification indicating that a scheduled captcha with the
  // given |captcha_id| must be solved for the given |payment_id| before the
  // user can continue to served ads.
  virtual void ShowScheduledCaptcha(const std::string& payment_id,
                                    const std::string& captcha_id) = 0;

  // Called to snooze the scheduled captcha, if any.
  virtual void SnoozeScheduledCaptcha() = 0;
#endif

  // Called when a notification ad with |placement_id| is shown.
  virtual void OnNotificationAdShown(const std::string& placement_id) = 0;

  // Called when a notification ad with |placement_id| is closed. |by_user| is
  // |true| if the user closed the notification otherwise |false|.
  virtual void OnNotificationAdClosed(const std::string& placement_id,
                                      const bool by_user) = 0;

  // Called when a notification ad with |placement_id| is clicked.
  virtual void OnNotificationAdClicked(const std::string& placement_id) = 0;

  // Called to get diagnostics to help identify issues. The callback takes two
  // arguments - |bool| is set to |true| if successful otherwise |false|.
  // |base::Value::List| containing info of the obtained diagnostics.
  virtual void GetDiagnostics(GetDiagnosticsCallback callback) = 0;

  // Called when the user changes the locale of their operating system. This
  // call is not required if the operating system restarts the browser when
  // changing the locale. |locale| should be specified in either
  // <ISO-639-1>-<ISO-3166-1> or <ISO-639-1>_<ISO-3166-1> format.
  virtual void OnChangeLocale(const std::string& locale) = 0;

  // Called when a resource component has been updated.
  virtual void OnResourceComponentUpdated(const std::string& id) = 0;

  // Called when a page navigation was initiated by a user gesture.
  // |page_transition_type| containing the page transition type, see enums for
  // |PageTransitionType|.
  virtual void OnUserGesture(const int32_t page_transition_type) = 0;

  // Called when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a chain of redirect URLs that
  // occurred for this navigation. |html| containing the page content as HTML.
  virtual void OnHtmlLoaded(const SessionID& tab_id,
                            const std::vector<GURL>& redirect_chain,
                            const std::string& html) = 0;

  // Called when the page for |tab_id| has loaded and the content is available
  // for analysis. |redirect_chain| containing a chain of redirect URLs that
  // occurred for this navigation. |text| containing the page content as text.
  virtual void OnTextLoaded(const SessionID& tab_id,
                            const std::vector<GURL>& redirect_chain,
                            const std::string& text) = 0;

  // Called when media starts playing on a browser tab for the specified
  // |tab_id|.
  virtual void OnMediaStart(const SessionID& tab_id) = 0;

  // Called when media stops playing on a browser tab for the specified
  // |tab_id|.
  virtual void OnMediaStop(const SessionID& tab_id) = 0;

  // Called when a browser tab is updated with the specified |url|. |is_active|
  // is set to |true| if |tab_id| refers to the currently active tab otherwise
  // is set to |false|. |is_browser_active| is set to |true| if the browser
  // window is active otherwise |false|.
  virtual void OnTabUpdated(const SessionID& tab_id,
                            const GURL& url,
                            const bool is_active,
                            const bool is_browser_active) = 0;

  // Called when a browser tab with the specified |tab_id| was closed.
  virtual void OnTabClosed(const SessionID& tab_id) = 0;

  // Called to get the statement of accounts. The callback takes five arguments
  // - |bool| is set to |true| if successful otherwise |false|. |double|
  // containing the next payment date which is the number of seconds since epoch
  // (Jan 1, 1970). |int| containing the number of ads received this month.
  // |double| containing the total earnings this month. |double| containing the
  // total earnings last month.
  virtual void GetStatementOfAccounts(
      GetStatementOfAccountsCallback callback) = 0;

  // Should be called to serve an inline content ad for the specified
  // |dimensions|. The callback takes three arguments - |bool| is set to |true|
  // if successful otherwise |false|, |std::string| containing the dimensions
  // and |base::DictionaryValue| containing the ad.
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
      const ads::mojom::InlineContentAdEventType event_type) = 0;

  // Called to prefetch the next new tab page ad.
  virtual void PrefetchNewTabPageAd() = 0;

  // Called to get a prefetched new tab page ad.
  virtual absl::optional<ads::NewTabPageAdInfo> GetPrefetchedNewTabPageAd() = 0;

  // Called when failing to prefetch a new tab page ad for |placement_id| and
  // |creative_instance_id|.
  virtual void OnFailedToPrefetchNewTabPageAd(
      const std::string& placement_id,
      const std::string& creative_instance_id) = 0;

  // Called when a user views or interacts with a new tab page ad to trigger an
  // |event_type| event for the specified |placement_id| and
  // |creative_instance_id|. |placement_id| should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same |placement_id|
  // generated for the viewed event should be used for all other events for the
  // same ad placement.
  virtual void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::NewTabPageAdEventType event_type) = 0;

  // Called when a user views or interacts with a promoted content ad to trigger
  // an |event_type| event for the specified |placement_id| and
  // |creative_instance_id|. |placement_id| should be a 128-bit random GUID in
  // the form of version 4. See RFC 4122, section 4.4. The same |placement_id|
  // generated for the viewed event should be used for all other events for the
  // same ad placement.
  virtual void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::PromotedContentAdEventType event_type) = 0;

  // Called when a user views or interacts with a search result ad to trigger an
  // |event_type| event for the ad specified in |ad_mojom|. The callback takes
  // three arguments - |bool| is set to |true| if successful otherwise |false|,
  // |std::string| containing the placement id and
  // |ads::mojom::SearchResultAdEventType| containing the event type. NOTE: You
  // must wait for the callback before calling another |kViewed| event to handle
  // frequency capping.
  virtual void TriggerSearchResultAdEvent(
      ads::mojom::SearchResultAdInfoPtr ad_mojom,
      const ads::mojom::SearchResultAdEventType event_type,
      TriggerSearchResultAdEventCallback callback) = 0;

  // Called to purge orphaned served ad events. NOTE: You should call before
  // triggering new ad events for the specified |ad_type|. The callback takes
  // one argument - |bool| is set to |true| if successful otherwise |false|.
  virtual void PurgeOrphanedAdEventsForType(
      const ads::mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) = 0;

  // Called to get history between |from_time| and |to_time| date range. The
  // callback takes one argument - |base::ListValue| containing info of the
  // obtained history.
  virtual void GetHistory(const base::Time from_time,
                          const base::Time to_time,
                          GetHistoryCallback callback) = 0;

  // Called to like an advertiser. This is a toggle, so calling it again returns
  // the setting to the neutral state. The callback takes one argument -
  // |std::string| containing the current state.
  virtual void ToggleAdThumbUp(const std::string& json,
                               ToggleAdThumbUpCallback callback) = 0;

  // Called to dislike an advertiser. This is a toggle, so calling it again
  // returns the setting to the neutral state. The callback takes one argument -
  // |std::string| containing the current state.
  virtual void ToggleAdThumbDown(const std::string& json,
                                 ToggleAdThumbDownCallback callback) = 0;

  // Called to no longer receive ads for the specified category. This is a
  // toggle, so calling it again returns the setting to the neutral state. The
  // callback takes two arguments - |std::string| containing the category. |int|
  // containing the action.
  virtual void ToggleAdOptIn(const std::string& category,
                             const int action,
                             ToggleAdOptInCallback callback) = 0;

  // Called to receive ads for the specified category. This is a toggle, so
  // calling it again returns the setting to the neutral state. The callback
  // takes two arguments - |std::string| containing the category. |int|
  // containing the action.
  virtual void ToggleAdOptOut(const std::string& category,
                              const int action,
                              ToggleAdOptOutCallback callback) = 0;

  // Called to save an ad for later viewing. This is a toggle, so calling it
  // again removes the ad from the saved list. The callback takes one argument -
  // |std::string| containing the current state.
  virtual void ToggleSavedAd(const std::string& json,
                             ToggleSavedAdCallback callback) = 0;

  // Called to mark an ad as inappropriate. This is a toggle, so calling it
  // again unmarks the ad. The callback takes one argument - |std::string|
  // containing the current state.
  virtual void ToggleFlaggedAd(const std::string& json,
                               ToggleFlaggedAdCallback callback) = 0;

  // Called to wipe all state.
  virtual void WipeState(const bool should_shutdown) = 0;

 protected:
  base::ObserverList<AdsServiceObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
