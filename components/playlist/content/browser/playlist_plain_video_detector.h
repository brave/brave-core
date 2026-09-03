/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_PLAIN_VIDEO_DETECTOR_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_PLAIN_VIDEO_DETECTOR_H_

#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/playlist/content/browser/playlist_network_observer.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace content {
struct GlobalRenderFrameHostToken;
class WebContents;
}  // namespace content

namespace playlist {

// Smallest possible slice of the network-observation based detector: it only
// recognizes a plain `<video src="...">`/`<audio src="...">` load (a whole
// file requested by the media element itself, not an MSE fragment or a
// manifest) and forwards it through the same callback signature the existing
// DOM-scraping detector already uses. HLS/DASH/MSE support builds on top of
// this in later patches; this one exists to let reviewers evaluate the
// network-observation approach against the simplest case first.
class PlaylistPlainVideoDetector
    : public content::WebContentsUserData<PlaylistPlainVideoDetector>,
      public content::WebContentsObserver,
      public PlaylistNetworkObserver::Observer {
 public:
  using MediaDetectedCallback =
      base::RepeatingCallback<void(GURL, std::vector<mojom::PlaylistItemPtr>)>;

  static void CreateForWebContents(content::WebContents* web_contents,
                                   MediaDetectedCallback callback);

  PlaylistPlainVideoDetector(const PlaylistPlainVideoDetector&) = delete;
  PlaylistPlainVideoDetector& operator=(const PlaylistPlainVideoDetector&) =
      delete;
  ~PlaylistPlainVideoDetector() override;

  // PlaylistNetworkObserver::Observer:
  void OnMediaResponseObserved(
      const content::GlobalRenderFrameHostToken& frame_token,
      const MediaResponseInfo& info) override;

 private:
  friend class content::WebContentsUserData<PlaylistPlainVideoDetector>;

  PlaylistPlainVideoDetector(content::WebContents* web_contents,
                             MediaDetectedCallback callback);

  raw_ptr<PlaylistNetworkObserver> network_observer_;
  MediaDetectedCallback callback_;

  // Only needed so a page that reloads the same <video src> doesn't produce a
  // duplicate item every time; PlaylistNetworkObserver's own dedup is
  // per-BrowserContext and short-lived, not per-tab.
  base::flat_set<GURL> notified_urls_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_PLAIN_VIDEO_DETECTOR_H_
