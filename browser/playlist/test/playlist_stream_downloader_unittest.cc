/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_stream_downloader.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {

namespace {

// A multivariant playlist whose single variant lives on a different host
// than the master itself - exactly the shape of a CDN that fronts playback
// through a director/redirect host (e.g. Dailymotion) before handing off to
// the host that actually serves the stream.
constexpr char kMasterPlaylist[] = R"(#EXTM3U
#EXT-X-STREAM-INF:BANDWIDTH=1000000
https://cdn.example.com/videos/abc/def/manifest.m3u8
)";

// This playlist's segment is relative, and must resolve against *this*
// playlist's own URL (https://cdn.example.com/videos/abc/def/manifest.m3u8),
// not the master's. Resolving against the wrong base lands on the master's
// host entirely, since master.example.com/entry.m3u8 has no path segments
// left to consume the "../../".
constexpr char kMediaPlaylist[] = R"(#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:4
#EXT-X-PLAYLIST-TYPE:VOD
#EXTINF:4,
../../frag/segment1.ts
#EXT-X-ENDLIST
)";

}  // namespace

class PlaylistStreamDownloaderTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);

    downloader_ =
        std::make_unique<PlaylistStreamDownloader>(&browser_context_);
    downloader_->SetURLLoaderFactoryForTesting(shared_url_loader_factory_);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  content::TestBrowserContext browser_context_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<PlaylistStreamDownloader> downloader_;
};

TEST_F(PlaylistStreamDownloaderTest,
       ResolvesRenditionSegmentsAgainstTheRenditionsOwnUrl) {
  test_url_loader_factory_.AddResponse(
      "https://master.example.com/entry.m3u8", kMasterPlaylist);
  test_url_loader_factory_.AddResponse(
      "https://cdn.example.com/videos/abc/def/manifest.m3u8",
      kMediaPlaylist);
  // Only registered at the correctly-resolved location. If the downloader
  // resolves the segment against the master's URL instead, this response
  // won't be found and the download will fail.
  test_url_loader_factory_.AddResponse(
      "https://cdn.example.com/videos/frag/segment1.ts", "segment-bytes");

  base::expected<PlaylistStreamDownloader::Result,
                 PlaylistStreamDownloader::Error>
      result;
  bool done = false;
  downloader_->Start(GURL("https://master.example.com/entry.m3u8"),
                     GURL("https://master.example.com/entry.m3u8"),
                     temp_dir_.GetPath(), base::DoNothing(),
                     base::BindLambdaForTesting(
                         [&](base::expected<PlaylistStreamDownloader::Result,
                                            PlaylistStreamDownloader::Error>
                                 downloaded) {
                           result = std::move(downloaded);
                           done = true;
                         }));

  ASSERT_TRUE(base::test::RunUntil([&] { return done; }));
  ASSERT_TRUE(result.has_value())
      << "download failed with: "
      << PlaylistStreamDownloader::ErrorToString(result.error());

  std::string segment_contents;
  ASSERT_TRUE(base::ReadFileToString(
      temp_dir_.GetPath().AppendASCII("v_00000.ts"), &segment_contents));
  EXPECT_EQ("segment-bytes", segment_contents);
}

}  // namespace playlist
