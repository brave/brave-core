/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/playlist/content/browser/playlist_media_handler.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace playlist {

class PlaylistService;

// `PlaylistBackgroundWebContentsHelper` is attached to a background
// `WebContents` (see `PlaylistBackgroundWebContentses`). It's responsible for
// joining legacy and V2 detector results for the background `WebContents`, and
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

  PlaylistBackgroundWebContentsHelper(
      content::WebContents* web_contents,
      PlaylistService* service,
      PlaylistMediaHandler::OnceCallback on_media_detected_callback);

  // Runs `on_media_detected_callback_` for the first non-empty batch, whether
  // that came from the script or from the network.
  void OnMediaDetected(GURL url, std::vector<mojom::PlaylistItemPtr> items);

  // content::WebContentsObserver:
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;

  raw_ptr<PlaylistService> service_;
  PlaylistMediaHandler::OnceCallback on_media_detected_callback_;

  base::WeakPtrFactory<PlaylistBackgroundWebContentsHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_BACKGROUND_WEB_CONTENTS_HELPER_H_
