/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_NETWORK_MEDIA_DETECTOR_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_NETWORK_MEDIA_DETECTOR_H_

#include <memory>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/playlist/content/browser/playlist_media_session_observer.h"
#include "brave/components/playlist/content/browser/playlist_network_observer.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace playlist {

// Playlist's V2 media detection: media URLs come from the network stack
// (`PlaylistNetworkObserver`) and their title, author, artwork and duration
// come from the page's MediaSession. Nothing is injected into the page.
//
// Emits through the same `(page URL, items)` callback the JS path uses, so
// consumers can't tell the two apart, and `PlaylistTabHelper` dedupes items
// that both paths happen to find.
class PlaylistNetworkMediaDetector final
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PlaylistNetworkMediaDetector>,
      public PlaylistNetworkObserver::Observer {
 public:
  using MediaDetectedCallback =
      base::RepeatingCallback<void(GURL, std::vector<mojom::PlaylistItemPtr>)>;

  PlaylistNetworkMediaDetector(const PlaylistNetworkMediaDetector&) = delete;
  PlaylistNetworkMediaDetector& operator=(const PlaylistNetworkMediaDetector&) =
      delete;
  ~PlaylistNetworkMediaDetector() override;

  // PlaylistNetworkObserver::Observer:
  void OnMediaResponseObserved(
      const content::GlobalRenderFrameHostToken& frame_token,
      const MediaResponseInfo& info) override;

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;

 private:
  friend class content::WebContentsUserData<PlaylistNetworkMediaDetector>;

  PlaylistNetworkMediaDetector(content::WebContents* web_contents,
                               MediaDetectedCallback on_media_detected);

  void OnMetadataChanged();
  void Emit();
  mojom::PlaylistItemPtr MakeItem(const GURL& page_url,
                                  const GURL& media_url) const;

  // Media found on the current page but not handed over yet.
  std::vector<GURL> pending_media_;
  // Media already handed over for the current page.
  base::flat_set<GURL> emitted_media_;

  // MediaSession metadata lands after the first media bytes do, so hold newly
  // found media briefly rather than emitting an untitled item.
  base::OneShotTimer emit_timer_;

  std::unique_ptr<PlaylistMediaSessionObserver> media_session_observer_;
  raw_ptr<PlaylistNetworkObserver> network_observer_ = nullptr;
  MediaDetectedCallback on_media_detected_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_NETWORK_MEDIA_DETECTOR_H_
