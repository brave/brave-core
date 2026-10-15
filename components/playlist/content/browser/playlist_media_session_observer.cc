/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_media_session_observer.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/containers/map_util.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/web_contents.h"
#include "url/url_constants.h"

namespace playlist {

namespace {

// Picks the largest artwork the page offered. Sites commonly list several
// sizes; the biggest is the one worth keeping as a thumbnail.
GURL PickArtwork(const std::vector<media_session::MediaImage>& images) {
  const media_session::MediaImage* best = nullptr;
  int best_area = -1;
  for (const auto& image : images) {
    if (!image.src.SchemeIs(url::kHttpsScheme)) {
      continue;
    }

    // An image with no declared sizes still beats nothing at all.
    int area = 0;
    for (const auto& size : image.sizes) {
      area = std::max(area, size.GetArea());
    }

    if (area > best_area) {
      best_area = area;
      best = &image;
    }
  }

  return best ? best->src : GURL();
}

}  // namespace

bool PlaylistMediaMetadata::empty() const {
  return title.empty() && artist.empty() && artwork.is_empty() &&
         duration.is_zero();
}

PlaylistMediaSessionObserver::PlaylistMediaSessionObserver(
    content::WebContents* web_contents,
    base::RepeatingClosure on_metadata_changed)
    : on_metadata_changed_(std::move(on_metadata_changed)) {
  CHECK(web_contents);
  CHECK(on_metadata_changed_);

  content::MediaSession::Get(web_contents)
      ->AddObserver(receiver_.BindNewPipeAndPassRemote());
}

PlaylistMediaSessionObserver::~PlaylistMediaSessionObserver() = default;

void PlaylistMediaSessionObserver::Reset() {
  metadata_ = PlaylistMediaMetadata();
}

void PlaylistMediaSessionObserver::MediaSessionMetadataChanged(
    const std::optional<media_session::MediaMetadata>& metadata) {
  if (!metadata) {
    return;
  }

  metadata_.title = metadata->title;
  // `artist` is what sites use for a channel or uploader name; `source_title`
  // is the site itself, which is a poor author but better than nothing.
  metadata_.artist =
      metadata->artist.empty() ? metadata->source_title : metadata->artist;
  on_metadata_changed_.Run();
}

void PlaylistMediaSessionObserver::MediaSessionImagesChanged(
    const base::flat_map<media_session::mojom::MediaSessionImageType,
                         std::vector<media_session::MediaImage>>& images) {
  const auto* artwork = base::FindOrNull(
      images, media_session::mojom::MediaSessionImageType::kArtwork);
  if (!artwork) {
    return;
  }

  if (GURL picked = PickArtwork(*artwork); picked.is_valid()) {
    metadata_.artwork = std::move(picked);
    on_metadata_changed_.Run();
  }
}

void PlaylistMediaSessionObserver::MediaSessionPositionChanged(
    const std::optional<media_session::MediaPosition>& position) {
  if (!position) {
    return;
  }

  // Live streams report a max duration; there's nothing to save there.
  const base::TimeDelta duration = position->duration();
  if (duration.is_zero() || duration.is_max() || duration.is_inf()) {
    return;
  }

  if (metadata_.duration != duration) {
    metadata_.duration = duration;
    on_metadata_changed_.Run();
  }
}

}  // namespace playlist
