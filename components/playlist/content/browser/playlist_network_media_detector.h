/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_NETWORK_MEDIA_DETECTOR_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_NETWORK_MEDIA_DETECTOR_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/playlist/content/browser/playlist_media_session_observer.h"
#include "brave/components/playlist/content/browser/playlist_network_observer.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace playlist {

// Playlist's V2 media detection: media URLs come from the network stack
// (`PlaylistNetworkObserver`) and their title, author, artwork and duration
// come from the page's MediaSession. Nothing is injected into the page.
//
// Emits through the same `(page URL, items)` callback the JS path uses, so
// consumers can't tell the two apart, and `PlaylistTabHelper` dedupes items
// that both paths happen to find.
//
// An HLS master playlist's video-only and audio-only renditions are fetched
// separately by the page and would otherwise show up as their own, silently
// unplayable items; see `MaybeDiscoverHlsRenditions()`.
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

  void SetURLLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  friend class content::WebContentsUserData<PlaylistNetworkMediaDetector>;

  PlaylistNetworkMediaDetector(content::WebContents* web_contents,
                               MediaDetectedCallback on_media_detected);

  void OnMetadataChanged();
  void Emit();
  mojom::PlaylistItemPtr MakeItem(const GURL& page_url,
                                  const GURL& media_url) const;

  // HLS master playlists reference video-only and audio-only renditions that
  // the network stack observes as their own responses. Fetches `manifest_url`
  // once and, if it's a master playlist, adds every rendition it points to to
  // `suppressed_media_` so they never surface as their own items - they're
  // muxed back together when the master itself is saved.
  void MaybeDiscoverHlsRenditions(const GURL& manifest_url);
  void OnHlsMasterPlaylistFetched(const GURL& manifest_url,
                                  network::SimpleURLLoader* loader,
                                  std::optional<std::string> body);

  // Media found on the current page but not handed over yet.
  std::vector<GURL> pending_media_;
  // Media already handed over for the current page.
  base::flat_set<GURL> emitted_media_;
  // Renditions of an HLS master playlist found on this page. Never surfaced
  // as their own item - see `MaybeDiscoverHlsRenditions()`.
  base::flat_set<GURL> suppressed_media_;
  // Master playlist URLs already fetched (or being fetched) to find
  // renditions, so the same master isn't parsed twice.
  base::flat_set<GURL> discovered_masters_;

  // MediaSession metadata lands after the first media bytes do, so hold newly
  // found media briefly rather than emitting an untitled item.
  base::OneShotTimer emit_timer_;

  std::unique_ptr<PlaylistMediaSessionObserver> media_session_observer_;
  raw_ptr<PlaylistNetworkObserver> network_observer_ = nullptr;
  MediaDetectedCallback on_media_detected_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::map<network::SimpleURLLoader*, std::unique_ptr<network::SimpleURLLoader>>
      in_flight_manifest_fetches_;

  base::WeakPtrFactory<PlaylistNetworkMediaDetector> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_NETWORK_MEDIA_DETECTOR_H_
