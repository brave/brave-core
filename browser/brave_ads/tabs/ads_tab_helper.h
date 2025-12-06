/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TABS_ADS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TABS_ADS_TAB_HELPER_H_

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/browser/service/network_client.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser_list_observer.h"
#endif

class Browser;
class GURL;

namespace content {
struct MediaPlayerId;
}  // namespace content

namespace brave_ads {

class AdsService;

class AdsTabHelper final : public content::WebContentsObserver,
#if !BUILDFLAG(IS_ANDROID)
                           public BrowserListObserver,
#endif
                           public content::WebContentsUserData<AdsTabHelper> {
 public:
  explicit AdsTabHelper(content::WebContents*);

  AdsTabHelper(const AdsTabHelper&) = delete;
  AdsTabHelper& operator=(const AdsTabHelper&) = delete;

  ~AdsTabHelper() override;

  AdsService* ads_service() { return ads_service_; }

  void SetSearchWidgetAsEntryPoint(bool is_search_widget_entry_point) {
    is_search_widget_entry_point_ = is_search_widget_entry_point;
  }

  void SetAdsServiceForTesting(AdsService* ads_service);

 private:
  friend class content::WebContentsUserData<AdsTabHelper>;

  bool UserHasJoinedBraveRewards() const;
  bool UserHasOptedInToNotificationAds() const;

  bool IsVisible() const;

  void MaybeSetBrowserIsActive();
  void MaybeSetBrowserIsNoLongerActive();

  void ProcessNavigation();
  void ProcessSameDocumentNavigation();
  void ResetNavigationState();

  void MaybeNotifyBrowserDidBecomeActive();
  void MaybeNotifyBrowserDidResignActive();

  void MaybeNotifyUserGestureEventTriggered(
      content::NavigationHandle* navigation_handle);

  void MaybeNotifyTabDidChange();

  void MaybeNotifyTabDidLoad();

  bool ShouldNotifyTabContentDidChange() const;
  void MaybeNotifyTabHtmlContentDidChange();
  void OnMaybeNotifyTabHtmlContentDidChange(
      const std::vector<GURL>& redirect_chain,
      base::Value value);
  void MaybeNotifyTabTextContentDidChange();
  void OnMaybeNotifyTabTextContentDidChange(
      const std::vector<GURL>& redirect_chain,
      base::Value value);

  bool IsPlayingMedia(const std::string& media_player_uuid);
  void MaybeNotifyTabDidStartPlayingMedia();
  void MaybeNotifyTabDidStopPlayingMedia();

  void MaybeNotifyTabdidClose();

  // content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void MediaStartedPlaying(const MediaPlayerInfo& video_type,
                           const content::MediaPlayerId& id) override;
  void MediaStoppedPlaying(
      const MediaPlayerInfo& video_type,
      const content::MediaPlayerId& id,
      WebContentsObserver::MediaStoppedReason reason) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;

#if !BUILDFLAG(IS_ANDROID)
  // BrowserListObserver:
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;
#endif

  void MaybeReportSearchQueryMetric(
      content::NavigationHandle* navigation_handle);
  void ReportSearchQueryMetric(const std::string& payload);
  void ReportSearchQueryMetricCallback(
      mojom::UrlResponseInfoPtr mojom_url_response);
  std::optional<std::string> MaybeBuildSearchQueryMetricPayload(
      const GURL& url,
      ui::PageTransition page_transition);
  std::optional<std::string> Country() const;
  std::string CreatedAt() const;
  std::optional<std::string> DefaultSearchEngine() const;
  std::string EntryPoint(ui::PageTransition page_transition);
  bool IsDefaultBrowser() const;
  bool IsFirstQuery() const;
  std::string Language() const;
  std::string Platform() const;
  std::optional<std::string> SearchEngine(const GURL& url) const;
  std::string TransactionId() const;
  void LogEntryPointForDebugging(ui::PageTransition page_transition) const;
  bool is_search_widget_entry_point_ = false;
  bool is_bookmark_entry_point_ = false;
  std::unique_ptr<NetworkClient> network_client_;

  SessionID session_id_;

  raw_ptr<AdsService> ads_service_ = nullptr;  // Not owned.

  bool is_web_contents_visible_ = false;

  bool was_restored_ = false;
  bool is_new_navigation_ = false;
  std::vector<GURL> redirect_chain_;
  std::optional<int> http_status_code_;

  std::set</*media_player_uuid*/ std::string> media_players_;

  std::optional<bool> is_browser_active_;

  base::WeakPtrFactory<AdsTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TABS_ADS_TAB_HELPER_H_
