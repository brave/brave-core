/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ADS_H_
#define BAT_ADS_ADS_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ads/ad_content_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/category_content_info.h"
#include "bat/ads/export.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"
#include "bat/ads/statement_info.h"

namespace ads {

using InitializeCallback = std::function<void(const Result)>;
using ShutdownCallback = std::function<void(const Result)>;

using RemoveAllHistoryCallback = std::function<void(const Result)>;

using GetTransactionHistoryCallback =
    std::function<void(const bool, const StatementInfo&)>;

// |_environment| indicates that URL requests should use production, staging or
// development servers but can be overridden via command-line arguments
extern Environment _environment;

// |_build_channel| indicates the build channel
extern BuildChannel _build_channel;

// |_is_debug| indicates that the next catalog download should be reduced from
// ~1 hour to ~25 seconds. This value should be set to |false| on production
// builds and |true| on debug builds but can be overridden via command-line
// arguments
extern bool _is_debug;

// Catalog schema resource id
extern const char _catalog_schema_resource_id[];

// Returns |true| if the locale is supported; otherwise returns |false|
bool IsSupportedLocale(
    const std::string& locale);

// Returns |true| if the locale is newly supported; otherwise returns |false|
bool IsNewlySupportedLocale(
    const std::string& locale,
    const int last_schema_version);

class ADS_EXPORT Ads {
 public:
  Ads() = default;
  virtual ~Ads() = default;

  static Ads* CreateInstance(
      AdsClient* ads_client);

  // Should be called to initialize ads, i.e. when launching the browser or when
  // ads is implicitly enabled by a user on the client. The callback takes one
  // argument — |Result| should be set to |SUCCESS| if successful; otherwise,
  // should be set to |FAILED|
  virtual void Initialize(
      InitializeCallback callback) = 0;

  // Should be called to shutdown ads when a user implicitly disables ads.
  // Shutting down ads will call |CloseNotification| for each ad notification in
  // the Notification Center on the client. The callback takes one argument —
  // |Result| should be set to |SUCCESS| if successful; otherwise, should be set
  // to |FAILED|
  virtual void Shutdown(
      ShutdownCallback callback) = 0;

  // Should be called when the user implicitly changes the locale of their
  // operating system. This call is not required if the operating system
  // restarts the browser when changing locale. |locale| should be specified in
  // any of the following formats:
  //
  //     <language>-<REGION> i.e. en-US
  //     <language>-<REGION>.<ENCODING> i.e. en-US.UTF-8
  //     <language>_<REGION> i.e. en_US
  //     <language>-<REGION>.<ENCODING> i.e. en_US.UTF-8
  virtual void ChangeLocale(
      const std::string& locale) = 0;

  // Should be called when the ads subdivision targeting code has changed
  virtual void OnAdsSubdivisionTargetingCodeHasChanged() = 0;

  // Should be called when a page has loaded in a browser tab, and the HTML is
  // available for analysis
  virtual void OnPageLoaded(
    const int32_t tab_id,
      const std::string& original_url,
      const std::string& url,
      const std::string& html) = 0;

  // Should be called when a user is no longer idle. This call is optional for
  // mobile devices
  virtual void OnUnIdle() = 0;

  // Should be called when a user is idle for the specified threshold set in
  // |SetIdleThreshold|. This call is optional for mobile devices
  virtual void OnIdle() = 0;

  // Should be called when the browser enters the foreground
  virtual void OnForeground() = 0;

  // Should be called when the browser enters the background
  virtual void OnBackground() = 0;

  // Should be called to report when the media has started playing on the
  // browser tab specified by |tab_id|
  virtual void OnMediaPlaying(
      const int32_t tab_id) = 0;

  // Should be called to report when the media has stopped playing on the
  // browser tab specified by |tab_id|
  virtual void OnMediaStopped(
      const int32_t tab_id) = 0;

  // Should be called to report user activity on a browser tab specified by
  // |tab_id|. |is_active| should be set to |true| if |tab_id| refers to the
  // currently active tab; otherwise, should be set to |false|.
  // |is_browser_active| should be set to |true| if the current browser window
  // is active; otherwise, should be set to |false|. |is_incognito| should be
  // set to |true| if the tab is private; otherwise, should be set to |false|
  virtual void OnTabUpdated(
      const int32_t tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_browser_active,
      const bool is_incognito) = 0;

  // Should be called to report when a browser tab has been closed as specified
  // by |tab_id|
  virtual void OnTabClosed(
      const int32_t tab_id) = 0;

  // Should be called to report when the wallet has been updated
  virtual void OnWalletUpdated(
      const std::string& payment_id,
      const std::string& seed) = 0;

  // Should be called when user model has been updated in the
  // |BraveUserModelInstaller| component
  virtual void OnUserModelUpdated(
      const std::string& id) = 0;

  // Should be called to get the notification specified by |uuid|. Returns
  // |true| and |info| if the notification exists; otherwise, should return
  // |false|
  virtual bool GetAdNotification(
      const std::string& uuid,
      AdNotificationInfo* info) = 0;

  // Should be called when a user views, clicks or dismisses an ad notification;
  // or an ad notification times out
  virtual void OnAdNotificationEvent(
      const std::string& uuid,
      const AdNotificationEventType event_type) = 0;

  // Should be called when a user views or clicks a new tab page ad
  virtual void OnNewTabPageAdEvent(
      const std::string& wallpaper_id,
      const std::string& creative_instance_id,
      const NewTabPageAdEventType event_type) = 0;

  // Should be called to remove all cached history. The callback takes one
  // argument — |Result| should be set to |SUCCESS| if successful; otherwise,
  // should be set to |FAILED|
  virtual void RemoveAllHistory(
      RemoveAllHistoryCallback callback) = 0;

  // Should be called to reconcile ad rewards with the server, i.e. after an
  // ad grant is claimed
  virtual void ReconcileAdRewards() = 0;

  // Should be called to get ads history. Returns |AdsHistoryInfo|
  virtual AdsHistoryInfo GetAdsHistory(
      const AdsHistoryInfo::FilterType filter_type,
      const AdsHistoryInfo::SortType sort_type,
      const uint64_t from_timestamp,
      const uint64_t to_timestamp) = 0;

  // Should be called to get transaction history. The callback takes one
  // argument — |StatementInfo| which contains a list of |TransactionInfo|
  // transactions and associated earned ad rewards
  virtual void GetTransactionHistory(
      GetTransactionHistoryCallback callback) = 0;

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
  // calling it again removes the ad from the saved list. Returns |true| if the
  // ad was saved; otherwise, should return |false|
  virtual bool ToggleSaveAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool saved) = 0;

  // Should be called to flag an ad as inappropriate. This is a toggle, so
  // calling it again unflags the ad. Returns |true| if the ad was flagged;
  // otherwise returns |false|
  virtual bool ToggleFlagAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool flagged) = 0;

 private:
  // Not copyable, not assignable
  Ads(const Ads&) = delete;
  Ads& operator=(const Ads&) = delete;
};

}  // namespace ads

#endif  // BAT_ADS_ADS_H_
