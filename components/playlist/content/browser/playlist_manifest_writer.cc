/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_manifest_writer.h"

#include <algorithm>
#include <cmath>

#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"

namespace playlist {

namespace {

// `EXT-X-MAP` requires version 6; everything else we write is version 3.
constexpr int kVersionWithInitSegment = 7;
constexpr int kVersionWithoutInitSegment = 3;

constexpr char kAudioGroupId[] = "audio";

// RFC 8216 requires EXT-X-TARGETDURATION to be an integer no smaller than any
// segment's duration.
int ComputeTargetDuration(const std::vector<HlsSegmentEntry>& segments) {
  double longest = 0;
  for (const auto& segment : segments) {
    longest = std::max(longest, segment.duration.InSecondsF());
  }

  return std::max(1, static_cast<int>(std::ceil(longest)));
}

}  // namespace

std::string WriteHlsMediaPlaylist(const HlsRendition& rendition) {
  const int version = rendition.init_file_name ? kVersionWithInitSegment
                                               : kVersionWithoutInitSegment;

  std::string manifest = absl::StrFormat(
      "#EXTM3U\n"
      "#EXT-X-VERSION:%d\n"
      "#EXT-X-PLAYLIST-TYPE:VOD\n"
      "#EXT-X-TARGETDURATION:%d\n"
      "#EXT-X-MEDIA-SEQUENCE:0\n",
      version, ComputeTargetDuration(rendition.segments));

  if (rendition.init_file_name) {
    base::StrAppend(&manifest,
                    {"#EXT-X-MAP:URI=\"", *rendition.init_file_name, "\"\n"});
  }

  for (const auto& segment : rendition.segments) {
    absl::StrAppendFormat(&manifest, "#EXTINF:%.6f,\n",
                          segment.duration.InSecondsF());
    base::StrAppend(&manifest, {segment.file_name, "\n"});
  }

  base::StrAppend(&manifest, {"#EXT-X-ENDLIST\n"});
  return manifest;
}

std::string WriteHlsMultivariantPlaylist(const HlsMultivariant& multivariant) {
  std::string manifest = absl::StrFormat(
      "#EXTM3U\n"
      "#EXT-X-VERSION:%d\n",
      kVersionWithInitSegment);

  base::StrAppend(&manifest,
                  {"#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"", kAudioGroupId,
                   "\",NAME=\"audio\",DEFAULT=YES,AUTOSELECT=YES,URI=\"",
                   multivariant.audio_playlist_file_name, "\"\n"});

  // BANDWIDTH is required, even when we only ever offer one variant.
  absl::StrAppendFormat(
      &manifest, "#EXT-X-STREAM-INF:BANDWIDTH=%llu",
      static_cast<uint64_t>(std::max(multivariant.bandwidth, uint64_t{1})));

  if (multivariant.width && multivariant.height) {
    absl::StrAppendFormat(&manifest, ",RESOLUTION=%dx%d", *multivariant.width,
                          *multivariant.height);
  }
  if (!multivariant.codecs.empty()) {
    base::StrAppend(&manifest, {",CODECS=\"", multivariant.codecs, "\""});
  }

  base::StrAppend(&manifest, {",AUDIO=\"", kAudioGroupId, "\"\n",
                              multivariant.video_playlist_file_name, "\n"});
  return manifest;
}

}  // namespace playlist
