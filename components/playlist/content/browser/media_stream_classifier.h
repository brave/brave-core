/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_MEDIA_STREAM_CLASSIFIER_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_MEDIA_STREAM_CLASSIFIER_H_

#include <optional>
#include <string_view>

#include "services/network/public/mojom/fetch_api.mojom-shared.h"

class GURL;

namespace playlist {

// How a media network response should be consumed by Playlist.
enum class MediaKind {
  // A complete media file that can be downloaded as-is.
  kProgressive,
  // An HLS playlist. May be multivariant or a media playlist; the downloader
  // tells those apart while parsing.
  kHlsManifest,
  // A DASH media presentation description.
  kDashManifest,
  // One piece of a larger stream, fed to MSE by the page. Useless on its own -
  // it only tells us that a stream exists at this origin.
  kSegment,
};

// Classifies a response as media, using only what's available at
// response-start. Returns nullopt for anything that isn't media.
//
// `mime_type` may be empty or generic, in which case the URL extension is
// consulted. `destination` is what separates a progressive file from an MSE
// fragment: Blink range-requests a plain <video src> just as a player
// range-requests a fragment, so the response headers alone can't tell them
// apart, but a media element load carries a video/audio destination while a
// player's fetch()/XHR does not.
std::optional<MediaKind> ClassifyMediaResponse(
    const GURL& url,
    std::string_view mime_type,
    network::mojom::RequestDestination destination);

// Whether `url` names an HLS manifest, judged by its path suffix alone. This
// deliberately mirrors `media::DemuxerManager::IsManifestDemuxerURL()`, which
// is how Chromium itself decides to use the HLS demuxer - if the two ever
// disagree, a stream would be saved as HLS but not played as HLS.
bool IsHlsManifestUrl(const GURL& url);

// Whether `url` names a stream manifest of any kind Playlist can download -
// HLS or DASH. Used to route a save to the stream downloader.
bool IsStreamManifestUrl(const GURL& url);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_MEDIA_STREAM_CLASSIFIER_H_
