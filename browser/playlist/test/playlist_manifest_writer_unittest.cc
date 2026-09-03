/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_manifest_writer.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace playlist {

TEST(PlaylistManifestWriterTest, MediaPlaylistWithoutInitSegment) {
  HlsRendition rendition;
  rendition.segments = {
      {"v_00000.ts", base::Milliseconds(2736)},
      {"v_00001.ts", base::Milliseconds(1500)},
  };

  EXPECT_EQ(
      "#EXTM3U\n"
      "#EXT-X-VERSION:3\n"
      "#EXT-X-PLAYLIST-TYPE:VOD\n"
      "#EXT-X-TARGETDURATION:3\n"
      "#EXT-X-MEDIA-SEQUENCE:0\n"
      "#EXTINF:2.736000,\n"
      "v_00000.ts\n"
      "#EXTINF:1.500000,\n"
      "v_00001.ts\n"
      "#EXT-X-ENDLIST\n",
      WriteHlsMediaPlaylist(rendition));
}

TEST(PlaylistManifestWriterTest, MediaPlaylistWithInitSegment) {
  HlsRendition rendition;
  rendition.init_file_name = "v_init.mp4";
  rendition.segments = {{"v_00000.m4s", base::Seconds(1)}};

  // EXT-X-MAP requires a later version than a plain TS playlist.
  EXPECT_EQ(
      "#EXTM3U\n"
      "#EXT-X-VERSION:7\n"
      "#EXT-X-PLAYLIST-TYPE:VOD\n"
      "#EXT-X-TARGETDURATION:1\n"
      "#EXT-X-MEDIA-SEQUENCE:0\n"
      "#EXT-X-MAP:URI=\"v_init.mp4\"\n"
      "#EXTINF:1.000000,\n"
      "v_00000.m4s\n"
      "#EXT-X-ENDLIST\n",
      WriteHlsMediaPlaylist(rendition));
}

TEST(PlaylistManifestWriterTest, TargetDurationRoundsUpAndIsNeverZero) {
  HlsRendition rendition;
  rendition.segments = {{"a.ts", base::Milliseconds(10)}};
  // RFC 8216 requires an integer no smaller than the longest segment, and a
  // zero target duration is rejected by parsers.
  EXPECT_NE(std::string::npos,
            WriteHlsMediaPlaylist(rendition).find("#EXT-X-TARGETDURATION:1\n"));

  rendition.segments = {{"a.ts", base::Milliseconds(9001)}};
  EXPECT_NE(std::string::npos, WriteHlsMediaPlaylist(rendition).find(
                                   "#EXT-X-TARGETDURATION:10\n"));
}

TEST(PlaylistManifestWriterTest, MultivariantBindsAudioToVideo) {
  HlsMultivariant multivariant;
  multivariant.video_playlist_file_name = "video.m3u8";
  multivariant.audio_playlist_file_name = "audio.m3u8";
  multivariant.codecs = "avc1.4d401f,mp4a.40.2";
  multivariant.bandwidth = 490622;
  multivariant.width = 1280;
  multivariant.height = 720;

  EXPECT_EQ(
      "#EXTM3U\n"
      "#EXT-X-VERSION:7\n"
      "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"audio\",NAME=\"audio\","
      "DEFAULT=YES,AUTOSELECT=YES,URI=\"audio.m3u8\"\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=490622,RESOLUTION=1280x720,"
      "CODECS=\"avc1.4d401f,mp4a.40.2\",AUDIO=\"audio\"\n"
      "video.m3u8\n",
      WriteHlsMultivariantPlaylist(multivariant));
}

TEST(PlaylistManifestWriterTest, MultivariantOmitsUnknownAttributes) {
  HlsMultivariant multivariant;
  multivariant.video_playlist_file_name = "video.m3u8";
  multivariant.audio_playlist_file_name = "audio.m3u8";

  const std::string manifest = WriteHlsMultivariantPlaylist(multivariant);
  EXPECT_EQ(std::string::npos, manifest.find("RESOLUTION"));
  EXPECT_EQ(std::string::npos, manifest.find("CODECS"));
  // BANDWIDTH is required even when we don't know it.
  EXPECT_NE(std::string::npos, manifest.find("BANDWIDTH=1,"));
}

}  // namespace playlist
