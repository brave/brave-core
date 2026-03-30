/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TABS_ADS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TABS_ADS_TAB_HELPER_H_

#include <optional>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser_list_observer.h"
#endif

class Browser;
class BrowserWindowInterface;
class GURL;


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

  void SetAdsServiceForTesting(AdsService* ads_service);

 private:
  friend class content::WebContentsUserData<AdsTabHelper>;

  bool UserHasOptedInToNotificationAds() const;

  bool IsVisible() const;

  void MaybeSetBrowserIsActive(const BrowserWindowInterface* browser_window);
  void MaybeSetBrowserIsNoLongerActive(
      const BrowserWindowInterface* browser_window);

  void ProcessNavigation();
  void ResetNavigationState();

  void MaybeNotifyBrowserDidBecomeActive();
  void MaybeNotifyBrowserDidResignActive();

  void MaybeNotifyUserGestureEventTriggered(
      content::NavigationHandle* navigation_handle);

  void MaybeNotifyTabDidChange();

  void MaybeNotifyTabDidLoad();

  bool ShouldNotifyTabContentDidChange() const;
  void MaybeNotifyTabTextContentDidChange();
  void OnMaybeNotifyTabTextContentDidChange(
      const std::vector<GURL>& redirect_chain,
      base::Value value);

  bool IsPlayingMediaWithAudio(const content::MediaPlayerId& id);
  void PlayerStartedPlayingWithAudio(const content::MediaPlayerId& id);
  void PlayerStoppedPlayingWithAudio(const content::MediaPlayerId& id);
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
  void MediaMutedStatusChanged(const content::MediaPlayerId& id,
                               bool muted) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;

#if !BUILDFLAG(IS_ANDROID)
  void MaybeSetInitialBrowserIsActive();

  // TODO(https://github.com/brave/brave-browser/issues/24970): Decouple
  // BrowserListObserver from AdsTabHelper.

  // BrowserListObserver:
  void OnBrowserRemoved(Browser* browser) override;
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;
#endif

  SessionID session_id_;

  raw_ptr<AdsService> ads_service_ = nullptr;  // Not owned.

  bool is_web_contents_visible_ = false;

  bool was_restored_ = false;
  bool is_new_navigation_ = false;
  std::vector<GURL> redirect_chain_;
  std::optional<int> http_status_code_;

  // Tracks media players that have started to guard against spurious
  // `MediaMutedStatusChanged` events for players that never actually started.
  base::flat_set<content::MediaPlayerId> started_media_players_;
  // Tracks media players that are currently playing with audio to determine
  // when to send start and stop playing notifications.
  base::flat_set<content::MediaPlayerId> media_players_with_audio_;

  std::optional<bool> is_browser_active_;

  base::WeakPtrFactory<AdsTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TABS_ADS_TAB_HELPER_H_
