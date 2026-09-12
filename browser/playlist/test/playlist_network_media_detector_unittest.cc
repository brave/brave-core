/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_network_media_detector.h"

#include <vector>

#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "brave/browser/playlist/test/playlist_unittest_base.h"
#include "brave/components/playlist/content/browser/playlist_network_observer.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {

namespace {

// Same shape as //brave/test/data/playlist/hls/demuxed/master.m3u8: one
// variant with a separate audio rendition (stream_1.m3u8), and a second,
// audio-only variant whose primary rendition is that same stream_1.m3u8.
constexpr char kMasterPlaylist[] = R"(#EXTM3U
#EXT-X-VERSION:7
#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="group_aud",NAME="audio_1",DEFAULT=YES,CHANNELS="2",URI="stream_1.m3u8"
#EXT-X-STREAM-INF:BANDWIDTH=490622,AVERAGE-BANDWIDTH=490622,RESOLUTION=1280x720,CODECS="avc1.4d401f,mp4a.40.2",AUDIO="group_aud"
stream_0.m3u8

#EXT-X-STREAM-INF:BANDWIDTH=66983,AVERAGE-BANDWIDTH=64091,CODECS="mp4a.40.2",AUDIO="group_aud"
stream_1.m3u8
)";

}  // namespace

class PlaylistNetworkMediaDetectorTest : public PlaylistUnitTestBase {
 public:
  void SetUp() override {
    PlaylistUnitTestBase::SetUp();
    NavigateAndCommit(GURL("https://x.com/A24/status/2096757842435084301"));

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);

    PlaylistNetworkMediaDetector::CreateForWebContents(
        web_contents(),
        base::BindLambdaForTesting(
            [this](GURL page_url, std::vector<mojom::PlaylistItemPtr> items) {
              for (const auto& item : items) {
                emitted_.push_back(item->media_source);
              }
            }));
    detector()->SetURLLoaderFactoryForTesting(shared_url_loader_factory_);
  }

  void TearDown() override {
    emitted_.clear();
    PlaylistUnitTestBase::TearDown();
  }

 protected:
  PlaylistNetworkMediaDetector* detector() {
    return PlaylistNetworkMediaDetector::FromWebContents(web_contents());
  }

  void ObserveManifest(const GURL& url) {
    PlaylistNetworkObserver::Get(browser_context())
        ->OnMediaResponse(
            main_rfh()->GetGlobalFrameToken(),
            MediaResponseInfo{.url = url,
                              .kind = MediaKind::kHlsManifest,
                              .mime_type = "application/vnd.apple.mpegurl"});
  }

  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::vector<GURL> emitted_;
};

TEST_F(PlaylistNetworkMediaDetectorTest,
       SuppressesRenditionsObservedAfterMaster) {
  const GURL master_url("https://cdn.example.com/master.m3u8");
  const GURL video_url("https://cdn.example.com/stream_0.m3u8");
  const GURL audio_url("https://cdn.example.com/stream_1.m3u8");
  test_url_loader_factory_.AddResponse(master_url.spec(), kMasterPlaylist);

  ObserveManifest(master_url);
  ASSERT_TRUE(base::test::RunUntil([&] { return !emitted_.empty(); }));

  // By the time the master's own item is emitted, its manifest has already
  // been fetched and parsed, so its renditions are known.
  ObserveManifest(video_url);
  ObserveManifest(audio_url);

  // Give a wrongly-surfaced rendition a chance to show up before asserting
  // that it doesn't.
  ASSERT_TRUE(base::test::RunUntil([&] { return emitted_.size() >= 1u; }));
  EXPECT_THAT(emitted_, testing::ElementsAre(master_url));
}

TEST_F(PlaylistNetworkMediaDetectorTest,
       DropsRenditionsQueuedBeforeMasterParseCompletes) {
  const GURL master_url("https://cdn.example.com/master.m3u8");
  const GURL video_url("https://cdn.example.com/stream_0.m3u8");
  const GURL audio_url("https://cdn.example.com/stream_1.m3u8");
  test_url_loader_factory_.AddResponse(master_url.spec(), kMasterPlaylist);

  // All three responses are observed back-to-back, before the master's
  // manifest fetch has had a chance to resolve.
  ObserveManifest(master_url);
  ObserveManifest(video_url);
  ObserveManifest(audio_url);

  ASSERT_TRUE(base::test::RunUntil([&] { return !emitted_.empty(); }));

  // Only the master should have been handed over - the renditions were
  // dropped from the queue once the master finished parsing.
  EXPECT_THAT(emitted_, testing::ElementsAre(master_url));
}

TEST_F(PlaylistNetworkMediaDetectorTest, DoesNotSuppressUnrelatedManifest) {
  const GURL master_url("https://cdn.example.com/master.m3u8");
  const GURL unrelated_url("https://cdn.example.com/other.m3u8");
  test_url_loader_factory_.AddResponse(master_url.spec(), kMasterPlaylist);

  ObserveManifest(master_url);
  ObserveManifest(unrelated_url);

  ASSERT_TRUE(base::test::RunUntil([&] { return emitted_.size() == 2u; }));
  EXPECT_THAT(emitted_,
              testing::UnorderedElementsAre(master_url, unrelated_url));
}

}  // namespace playlist
