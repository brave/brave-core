/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TABS_ADS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TABS_ADS_TAB_HELPER_H_

#include <optional>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"  // IWYU pragma: keep
#include "components/sessions/core/session_id.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser_list_observer.h"  // IWYU pragma: keep
#endif

class Browser;
class GURL;
class PrefService;

namespace brave_ads {

class AdsService;

class AdsTabHelper : public content::WebContentsObserver,
#if !BUILDFLAG(IS_ANDROID)
                     public BrowserListObserver,
#endif
                     public content::WebContentsUserData<AdsTabHelper> {
 public:
  explicit AdsTabHelper(content::WebContents*);
  ~AdsTabHelper() override;

  AdsTabHelper(const AdsTabHelper&) = delete;
  AdsTabHelper& operator=(const AdsTabHelper&) = delete;

 private:
  friend class content::WebContentsUserData<AdsTabHelper>;

  PrefService* GetPrefs() const;

  bool UserHasJoinedBraveRewards() const;

  bool IsVisible() const;

  void MaybeSetBrowserIsActive();
  void MaybeSetBrowserIsNoLongerActive();

  bool IsNewNavigation(content::NavigationHandle* navigation_handle);

  bool IsErrorPage(content::NavigationHandle* navigation_handle);

  void ProcessNavigation();

  void MaybeNotifyBrowserDidBecomeActive();
  void MaybeNotifyBrowserDidResignActive();

  void MaybeNotifyUserGestureEventTriggered(
      content::NavigationHandle* navigation_handle);

  void MaybeNotifyTabDidChange();

  void MaybeNotifyTabContentDidChange();
  void MaybeNotifyTabHtmlContentDidChange();
  void OnMaybeNotifyTabHtmlContentDidChange(
      const std::vector<GURL>& redirect_chain,
      base::Value value);
  void MaybeNotifyTabTextContentDidChange();
  void OnMaybeNotifyTabTextContentDidChange(
      const std::vector<GURL>& redirect_chain,
      base::Value value);

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

  SessionID tab_id_;

  raw_ptr<AdsService> ads_service_ = nullptr;  // NOT OWNED

  bool is_web_contents_visible_ = false;

  bool is_restoring_ = false;
  bool is_new_navigation_ = false;
  std::vector<GURL> redirect_chain_;
  bool is_error_page_ = false;

  std::optional<bool> is_browser_active_;

  base::WeakPtrFactory<AdsTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TABS_ADS_TAB_HELPER_H_
