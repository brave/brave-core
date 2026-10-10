/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_MEDIA_SESSION_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_MEDIA_SESSION_OBSERVER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/media_session/public/cpp/media_image.h"
#include "services/media_session/public/cpp/media_metadata.h"
#include "services/media_session/public/cpp/media_position.h"
#include "services/media_session/public/mojom/media_session.mojom.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}  // namespace content

namespace playlist {

// The subset of MediaSession metadata Playlist needs to describe an item.
struct PlaylistMediaMetadata {
  std::u16string title;
  std::u16string artist;
  GURL artwork;
  base::TimeDelta duration;

  bool empty() const;
};

// Watches a WebContents' MediaSession and keeps whatever the page published for
// the OS media controls. This is what lets us name, illustrate and time an item
// without injecting any script into the page - but it only works for sites that
// implement the MediaSession API, so callers must be ready for empty metadata.
class PlaylistMediaSessionObserver final
    : public media_session::mojom::MediaSessionObserver {
 public:
  PlaylistMediaSessionObserver(content::WebContents* web_contents,
                               base::RepeatingClosure on_metadata_changed);
  PlaylistMediaSessionObserver(const PlaylistMediaSessionObserver&) = delete;
  PlaylistMediaSessionObserver& operator=(const PlaylistMediaSessionObserver&) =
      delete;
  ~PlaylistMediaSessionObserver() override;

  const PlaylistMediaMetadata& metadata() const { return metadata_; }

  // Drops what the previous page published. The MediaSession outlives
  // navigations, but its metadata doesn't apply across them.
  void Reset();

  // media_session::mojom::MediaSessionObserver:
  void MediaSessionInfoChanged(
      media_session::mojom::MediaSessionInfoPtr info) override {}
  void MediaSessionMetadataChanged(
      const std::optional<media_session::MediaMetadata>& metadata) override;
  void MediaSessionActionsChanged(
      const std::vector<media_session::mojom::MediaSessionAction>& actions)
      override {}
  void MediaSessionImagesChanged(
      const base::flat_map<media_session::mojom::MediaSessionImageType,
                           std::vector<media_session::MediaImage>>& images)
      override;
  void MediaSessionPositionChanged(
      const std::optional<media_session::MediaPosition>& position) override;

 private:
  PlaylistMediaMetadata metadata_;
  base::RepeatingClosure on_metadata_changed_;

  mojo::Receiver<media_session::mojom::MediaSessionObserver> receiver_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_MEDIA_SESSION_OBSERVER_H_
