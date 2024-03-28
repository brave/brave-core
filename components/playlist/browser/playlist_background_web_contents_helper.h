/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_

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
  static void CreateForWebContents(
      content::WebContents* web_contents,
      PlaylistService* service,
      PlaylistMediaHandler::OnceCallback on_media_detected_callback);

  PlaylistBackgroundWebContentsHelper(
      const PlaylistBackgroundWebContentsHelper&) = delete;
  PlaylistBackgroundWebContentsHelper& operator=(
      const PlaylistBackgroundWebContentsHelper&) = delete;
  ~PlaylistBackgroundWebContentsHelper() override;

 private:
  friend class content::WebContentsUserData<
      PlaylistBackgroundWebContentsHelper>;
  using content::WebContentsUserData<
      PlaylistBackgroundWebContentsHelper>::CreateForWebContents;

  PlaylistBackgroundWebContentsHelper(content::WebContents* web_contents,
                                      PlaylistService* service);

  // content::WebContentsObserver:
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;

  raw_ptr<PlaylistService> service_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_
