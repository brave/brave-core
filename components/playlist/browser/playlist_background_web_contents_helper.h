/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_

#include "base/timer/timer.h"
#include "brave/components/playlist/browser/playlist_media_handler.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace playlist {

class PlaylistService;

// `PlaylistBackgroundWebContentsHelper` is attached to a background
// `WebContents` (see `PlaylistBackgroundWebContentses`). It's responsible for
// setting up the `PlaylistMediaHandler` for the background `WebContents`, and
// initializing renderer-side state (i.e. the scripts that
// `PlaylistRenderFrameObserver` injects into the page) via
// `WebContentsObserver::ReadyToCommitNavigation()`.
class PlaylistBackgroundWebContentsHelper final
    : public content::WebContentsUserData<PlaylistBackgroundWebContentsHelper>,
      public content::WebContentsObserver {
 public:
  PlaylistBackgroundWebContentsHelper(
      const PlaylistBackgroundWebContentsHelper&) = delete;
  PlaylistBackgroundWebContentsHelper& operator=(
      const PlaylistBackgroundWebContentsHelper&) = delete;
  ~PlaylistBackgroundWebContentsHelper() override;

 private:
  friend class content::WebContentsUserData<
      PlaylistBackgroundWebContentsHelper>;

  PlaylistBackgroundWebContentsHelper(
      content::WebContents* web_contents,
      PlaylistService* service,
      base::OnceCallback<void(GURL, bool)> callback);

  // content::WebContentsObserver:
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void GetLoadedUrl();

  raw_ptr<PlaylistService> service_;
  base::OnceCallback<void(GURL, bool)> callback_;
  base::OneShotTimer timer_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_
