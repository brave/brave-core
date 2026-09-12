/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_MANIFEST_WRITER_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_MANIFEST_WRITER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"

namespace playlist {

struct HlsSegmentEntry {
  // Relative to the manifest, so the whole item directory stays movable.
  std::string file_name;
  base::TimeDelta duration;
};

struct HlsRendition {
  // The `EXT-X-MAP` initialization segment, for fMP4 renditions.
  std::optional<std::string> init_file_name;
  std::vector<HlsSegmentEntry> segments;
};

// Writes a VOD media playlist naming locally saved segments.
std::string WriteHlsMediaPlaylist(const HlsRendition& rendition);

struct HlsMultivariant {
  std::string video_playlist_file_name;
  std::string audio_playlist_file_name;
  // Copied through from the source manifest so the demuxer can pick codecs
  // without probing. Either may be unset.
  std::string codecs;
  uint64_t bandwidth = 0;
  std::optional<int> width;
  std::optional<int> height;
};

// Writes a multivariant playlist that binds a video-only rendition to an
// audio-only one through an `EXT-X-MEDIA` group. This is how a stream whose
// audio and video arrived as separate files is made playable without muxing
// them: the HLS demuxer reads both and syncs them at playback.
std::string WriteHlsMultivariantPlaylist(const HlsMultivariant& multivariant);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_MANIFEST_WRITER_H_
