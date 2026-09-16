/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/media_stream_classifier.h"

#include <optional>
#include <string_view>

#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {

namespace {

using RequestDestination = network::mojom::RequestDestination;

std::optional<MediaKind> Classify(
    std::string_view url,
    std::string_view mime_type,
    RequestDestination destination = RequestDestination::kEmpty) {
  return ClassifyMediaResponse(GURL(url), mime_type, destination);
}

}  // namespace

TEST(MediaStreamClassifierTest, IgnoresNonMedia) {
  EXPECT_EQ(std::nullopt, Classify("https://a.com/", "text/html"));
  EXPECT_EQ(std::nullopt, Classify("https://a.com/a.js", "text/javascript"));
  EXPECT_EQ(std::nullopt, Classify("https://a.com/a.png", "image/png"));
  // An empty MIME type on a URL with no media extension tells us nothing.
  EXPECT_EQ(std::nullopt, Classify("https://a.com/watch", ""));
}

TEST(MediaStreamClassifierTest, IgnoresNonHttps) {
  EXPECT_EQ(std::nullopt,
            Classify("http://a.com/v.m3u8", "application/x-mpegurl"));
  EXPECT_EQ(std::nullopt, Classify("blob:https://a.com/abc", "video/mp4"));
  EXPECT_EQ(std::nullopt, Classify("http://a.com/v.mp4", "video/mp4",
                                   RequestDestination::kVideo));
}

TEST(MediaStreamClassifierTest, HlsManifestByMimeType) {
  for (std::string_view mime_type :
       {"application/x-mpegurl", "application/vnd.apple.mpegurl",
        "audio/x-mpegurl", "audio/mpegurl"}) {
    EXPECT_EQ(MediaKind::kHlsManifest, Classify("https://a.com/v", mime_type))
        << mime_type;
  }
  // MIME types are case insensitive.
  EXPECT_EQ(MediaKind::kHlsManifest,
            Classify("https://a.com/v", "APPLICATION/X-MPEGURL"));
}

TEST(MediaStreamClassifierTest, DashManifestByMimeType) {
  EXPECT_EQ(MediaKind::kDashManifest,
            Classify("https://a.com/v", "application/dash+xml"));
}

TEST(MediaStreamClassifierTest, ManifestByExtensionWhenMimeTypeIsUseless) {
  for (std::string_view mime_type :
       {"", "text/plain", "application/octet-stream"}) {
    EXPECT_EQ(MediaKind::kHlsManifest,
              Classify("https://a.com/hls/master.m3u8", mime_type))
        << mime_type;
    EXPECT_EQ(MediaKind::kDashManifest,
              Classify("https://a.com/dash/manifest.mpd", mime_type))
        << mime_type;
  }
}

TEST(MediaStreamClassifierTest, ManifestExtensionIgnoresQueryAndFragment) {
  EXPECT_EQ(MediaKind::kHlsManifest,
            Classify("https://a.com/master.m3u8?token=abc#t=1", "text/plain"));
  // A query that merely looks like an extension must not fool us.
  EXPECT_EQ(std::nullopt,
            Classify("https://a.com/watch?f=x.m3u8", "text/plain"));
}

TEST(MediaStreamClassifierTest, MediaByExtensionWhenMimeTypeIsUseless) {
  // CDNs routinely serve media as octet-stream. The extension has to carry it.
  EXPECT_EQ(MediaKind::kProgressive,
            Classify("https://a.com/v.mp4", "application/octet-stream",
                     RequestDestination::kVideo));
  EXPECT_EQ(MediaKind::kSegment,
            Classify("https://a.com/v.mp4", "application/octet-stream",
                     RequestDestination::kEmpty));
  // ...but an extensionless URL with a useless MIME type stays unclassified.
  EXPECT_EQ(std::nullopt,
            Classify("https://a.com/videoplayback", "application/octet-stream",
                     RequestDestination::kVideo));
}

TEST(MediaStreamClassifierTest, MediaElementLoadIsProgressive) {
  EXPECT_EQ(
      MediaKind::kProgressive,
      Classify("https://a.com/v.mp4", "video/mp4", RequestDestination::kVideo));
  EXPECT_EQ(MediaKind::kProgressive,
            Classify("https://a.com/a.mp3", "audio/mpeg",
                     RequestDestination::kAudio));
  // Blink range-requests progressive media too, so this must stay progressive.
  EXPECT_EQ(MediaKind::kProgressive,
            Classify("https://a.com/v.webm", "video/webm",
                     RequestDestination::kVideo));
}

TEST(MediaStreamClassifierTest, ScriptDrivenMediaFetchIsASegment) {
  // Same URL and MIME type as the progressive case above - only the
  // destination differs, which is exactly how MSE is detected.
  EXPECT_EQ(MediaKind::kSegment, Classify("https://a.com/v.mp4", "video/mp4",
                                          RequestDestination::kEmpty));
  EXPECT_EQ(MediaKind::kSegment, Classify("https://a.com/seg1.ts", "video/mp2t",
                                          RequestDestination::kEmpty));
  EXPECT_EQ(MediaKind::kSegment, Classify("https://a.com/seg1.m4s", "video/mp4",
                                          RequestDestination::kEmpty));
}

TEST(MediaStreamClassifierTest, ManifestWinsOverDestination) {
  // The built-in HLS demuxer fetches manifests with a video destination; a
  // JS player fetches them with none. Both are manifests.
  EXPECT_EQ(MediaKind::kHlsManifest,
            Classify("https://a.com/v.m3u8", "application/x-mpegurl",
                     RequestDestination::kVideo));
  EXPECT_EQ(MediaKind::kHlsManifest,
            Classify("https://a.com/v.m3u8", "application/x-mpegurl",
                     RequestDestination::kEmpty));
}

}  // namespace playlist
