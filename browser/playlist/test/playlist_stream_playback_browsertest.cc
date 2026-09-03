/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "base/token.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/playlist/content/browser/playlist_service.h"
#include "brave/components/playlist/content/browser/playlist_stream_downloader.h"
#include "brave/components/playlist/core/common/features.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {

namespace {

// The player WebUI, not the list UI: it's the one whose CSP grants
// `media-src chrome-untrusted://playlist-data`, and it's where saved media is
// actually played.
constexpr char kPlaylistPlayerWebUIURL[] =
    "chrome-untrusted://playlist-player/";

// Loads `url` into a <video> and resolves once metadata is known, then plays
// far enough to prove bytes actually reached the decoders. Returns a dict of
// what was observed, or a string starting with "error:".
constexpr char kPlayScript[] = R"(
  (async () => {
    const video = document.createElement('video');
    video.muted = true;
    document.body.appendChild(video);

    const failure = new Promise((_, reject) => {
      video.onerror = () => reject(
          'error: ' + (video.error ? video.error.message : 'unknown'));
    });

    const metadata = new Promise((resolve) => {
      video.onloadedmetadata = () => resolve();
    });

    video.src = $1;
    await Promise.race([metadata, failure]);

    // Metadata alone doesn't prove the segments are readable, so play until
    // the decoders have actually consumed some of both streams.
    const played = new Promise((resolve) => {
      video.ontimeupdate = () => {
        if (video.currentTime > 0.3) {
          resolve();
        }
      };
    });
    await video.play();
    await Promise.race([played, failure]);

    return {
      duration: video.duration,
      videoWidth: video.videoWidth,
      videoHeight: video.videoHeight,
      audioBytes: video.webkitAudioDecodedByteCount,
      videoBytes: video.webkitVideoDecodedByteCount,
    };
  })()
)";

}  // namespace

// Proves that a stream stored as segments plus a local manifest is playable
// through `PlaylistDataSource`, using Chromium's built-in HLS demuxer, and
// that HLS and DASH sources both get there. The separated audio/video case is
// the important one: it's the shape DASH and YouTube deliver, and nothing in
// the tree can mux those back together.
class PlaylistStreamPlaybackBrowserTest : public PlatformBrowserTest {
 public:
  PlaylistStreamPlaybackBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kPlaylist);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII("autoplay-policy",
                                    "no-user-gesture-required");
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    service_ =
        PlaylistServiceFactory::GetForBrowserContext(browser()->GetProfile());
    ASSERT_TRUE(service_);
    service_->SetUpForTesting();

    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_->Start());
  }

 protected:
  // Creates an item and copies `fixture` (a directory under
  // brave/test/data/playlist/hls) into its "hls" subdirectory, standing in for
  // what the downloader will produce. Returns the item id.
  std::string InstallStream(const std::string& fixture) {
    base::ScopedAllowBlockingForTesting allow_blocking;

    auto item = mojom::PlaylistItem::New();
    item->id = base::Token::CreateRandom().ToString();
    item->name = "test";
    item->page_source = GURL("https://example.com/");
    item->media_source = GURL("https://example.com/master.m3u8");
    item->media_path = item->media_source;
    service_->CreatePlaylistItem(item, /*cache=*/false);

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    const base::FilePath source = test_data_dir.AppendASCII("playlist")
                                      .AppendASCII("hls")
                                      .AppendASCII(fixture);
    CHECK(base::PathExists(source)) << source;

    const base::FilePath destination =
        service_->GetPlaylistItemDirPath(item->id).AppendASCII("hls");
    CHECK(base::CreateDirectory(destination.DirName()));
    CHECK(base::CopyDirectory(source, destination, /*recursive=*/true));

    return item->id;
  }

  GURL GetManifestURL(const std::string& id, const std::string& file) {
    return GURL(
        base::StrCat({"chrome-untrusted://playlist-data/", id, "/hls/", file}));
  }

  content::WebContents* NavigateToPlayerWebUI() {
    CHECK(content::NavigateToURL(
        browser()->tab_strip_model()->GetActiveWebContents(),
        GURL(kPlaylistPlayerWebUIURL)));
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  // `PlaylistService::CreatePlaylistItem` is private and befriends this
  // fixture, but not the per-test classes derived from it.
  void CreateItem(const mojom::PlaylistItemPtr& item, bool cache) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    service_->CreatePlaylistItem(item, cache);
  }

  // Creates an empty item and returns its id, along with the "hls" directory
  // the downloader should fill in.
  std::pair<std::string, base::FilePath> CreateEmptyStreamItem() {
    base::ScopedAllowBlockingForTesting allow_blocking;

    auto item = mojom::PlaylistItem::New();
    item->id = base::Token::CreateRandom().ToString();
    item->name = "test";
    item->page_source = GURL("https://example.com/");
    item->media_source = GURL("https://example.com/master.m3u8");
    item->media_path = item->media_source;
    service_->CreatePlaylistItem(item, /*cache=*/false);

    const base::FilePath dir =
        service_->GetPlaylistItemDirPath(item->id).AppendASCII("hls");
    CHECK(base::CreateDirectory(dir));
    return {item->id, dir};
  }

  base::expected<PlaylistStreamDownloader::Result,
                 PlaylistStreamDownloader::Error>
  DownloadStream(const GURL& manifest_url, const base::FilePath& dir) {
    PlaylistStreamDownloader downloader(browser()->GetProfile());
    base::test::TestFuture<base::expected<PlaylistStreamDownloader::Result,
                                          PlaylistStreamDownloader::Error>>
        future;
    downloader.Start(manifest_url, dir, base::DoNothing(),
                     future.GetCallback());
    return future.Take();
  }

  raw_ptr<PlaylistService> service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// The crux: audio and video live in separate files, referenced by one local
// manifest through an EXT-X-MEDIA audio rendition group.
IN_PROC_BROWSER_TEST_F(PlaylistStreamPlaybackBrowserTest,
                       DemuxedAudioAndVideo) {
  const std::string id = InstallStream("demuxed");
  auto* web_contents = NavigateToPlayerWebUI();

  auto result = content::EvalJs(
      web_contents,
      content::JsReplace(kPlayScript, GetManifestURL(id, "master.m3u8")));
  ASSERT_TRUE(result.is_ok()) << result.ExtractError();

  const base::DictValue& observed = result.ExtractDict();
  EXPECT_GT(*observed.FindDouble("duration"), 0.0);
  EXPECT_EQ(1280, *observed.FindInt("videoWidth"));
  EXPECT_EQ(720, *observed.FindInt("videoHeight"));
  // Both renditions had to be fetched and demuxed for these to be non-zero.
  EXPECT_GT(*observed.FindDouble("videoBytes"), 0.0);
  EXPECT_GT(*observed.FindDouble("audioBytes"), 0.0);
}

// The classic case: one variant with audio and video muxed into MPEG-TS.
IN_PROC_BROWSER_TEST_F(PlaylistStreamPlaybackBrowserTest,
                       MuxedTransportStream) {
  const std::string id = InstallStream("muxed");
  auto* web_contents = NavigateToPlayerWebUI();

  auto result = content::EvalJs(
      web_contents,
      content::JsReplace(kPlayScript, GetManifestURL(id, "media.m3u8")));
  ASSERT_TRUE(result.is_ok()) << result.ExtractError();

  const base::DictValue& observed = result.ExtractDict();
  EXPECT_GT(*observed.FindDouble("duration"), 0.0);
  EXPECT_EQ(1280, *observed.FindInt("videoWidth"));
  EXPECT_GT(*observed.FindDouble("videoBytes"), 0.0);
  EXPECT_GT(*observed.FindDouble("audioBytes"), 0.0);
}

// The item's "hls" directory is the security boundary for this route.
//
// Note that GURL canonicalizes a literal "../" out of the path before the data
// source ever sees it, so only percent-encoded separators actually reach the
// component checks - those are the ones worth testing here.
IN_PROC_BROWSER_TEST_F(PlaylistStreamPlaybackBrowserTest,
                       RejectsEncodedPathTraversal) {
  const std::string id = InstallStream("demuxed");
  auto* web_contents = NavigateToPlayerWebUI();

  for (const char* file : {"..%2Fthumbnail", "..%2F..%2Fmaster.m3u8",
                           "sub%2F..%2F..%2Fthumbnail"}) {
    auto result = content::EvalJs(
        web_contents,
        content::JsReplace(kPlayScript, GetManifestURL(id, file)));
    EXPECT_FALSE(result.is_ok()) << "should not have played: " << file;
  }
}

// An unknown item has no directory to serve from.
IN_PROC_BROWSER_TEST_F(PlaylistStreamPlaybackBrowserTest, RejectsUnknownItem) {
  auto* web_contents = NavigateToPlayerWebUI();

  auto result = content::EvalJs(
      web_contents,
      content::JsReplace(kPlayScript,
                         GetManifestURL("not-a-real-item", "master.m3u8")));
  EXPECT_FALSE(result.is_ok());
}

// End to end: fetch a real multivariant stream whose audio is a separate
// rendition, repackage it locally, and play what came out. This is the whole
// point of the design - the two renditions are never muxed.
IN_PROC_BROWSER_TEST_F(PlaylistStreamPlaybackBrowserTest,
                       DownloadsAndPlaysDemuxedStream) {
  auto [id, dir] = CreateEmptyStreamItem();

  auto result = DownloadStream(
      https_server_->GetURL("/playlist/hls/demuxed/master.m3u8"), dir);
  ASSERT_TRUE(result.has_value())
      << PlaylistStreamDownloader::ErrorToString(result.error());
  // A separate audio rendition means a local multivariant manifest is needed
  // to bind the two back together.
  EXPECT_EQ("master.m3u8", result->manifest_file_name);

  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("master.m3u8")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("video.m3u8")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("audio.m3u8")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("v_init.mp4")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("a_init.mp4")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("v_00000.m4s")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("a_00002.m4s")));
  }

  auto* web_contents = NavigateToPlayerWebUI();
  auto played = content::EvalJs(
      web_contents,
      content::JsReplace(kPlayScript,
                         GetManifestURL(id, result->manifest_file_name)));
  ASSERT_TRUE(played.is_ok()) << played.ExtractError();

  const base::DictValue& observed = played.ExtractDict();
  EXPECT_EQ(1280, *observed.FindInt("videoWidth"));
  EXPECT_GT(*observed.FindDouble("videoBytes"), 0.0);
  EXPECT_GT(*observed.FindDouble("audioBytes"), 0.0);
}

// The same, for a plain single-variant MPEG-TS stream.
IN_PROC_BROWSER_TEST_F(PlaylistStreamPlaybackBrowserTest,
                       DownloadsAndPlaysMuxedStream) {
  auto [id, dir] = CreateEmptyStreamItem();

  auto result = DownloadStream(
      https_server_->GetURL("/playlist/hls/muxed/media.m3u8"), dir);
  ASSERT_TRUE(result.has_value())
      << PlaylistStreamDownloader::ErrorToString(result.error());
  EXPECT_EQ("media.m3u8", result->manifest_file_name);

  auto* web_contents = NavigateToPlayerWebUI();
  auto played = content::EvalJs(
      web_contents,
      content::JsReplace(kPlayScript,
                         GetManifestURL(id, result->manifest_file_name)));
  ASSERT_TRUE(played.is_ok()) << played.ExtractError();
  EXPECT_GT(*played.ExtractDict().FindDouble("audioBytes"), 0.0);
}

IN_PROC_BROWSER_TEST_F(PlaylistStreamPlaybackBrowserTest,
                       ReportsNetworkErrorForMissingManifest) {
  auto [id, dir] = CreateEmptyStreamItem();

  auto result =
      DownloadStream(https_server_->GetURL("/playlist/hls/nope.m3u8"), dir);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(PlaylistStreamDownloader::Error::kNetwork, result.error());
}

// The whole Phase 3 path in one test: hand `PlaylistService` an item whose
// media source is a remote HLS manifest, let it download and repackage the
// stream, and play what it cached - exactly what saving a stream does.
IN_PROC_BROWSER_TEST_F(PlaylistStreamPlaybackBrowserTest,
                       ServiceCachesAndPlays) {
  auto item = mojom::PlaylistItem::New();
  item->id = base::Token::CreateRandom().ToString();
  item->name = "stream";
  item->page_source = GURL("https://example.com/");
  item->media_source =
      https_server_->GetURL("/playlist/hls/demuxed/master.m3u8");
  item->media_path = item->media_source;

  const std::string id = item->id;
  CreateItem(item, /*cache=*/true);

  // Caching is asynchronous and completes well after the item appears.
  ASSERT_TRUE(base::test::RunUntil([&] {
    return service_->HasPlaylistItem(id) &&
           service_->GetPlaylistItem(id)->cached;
  }));

  auto cached = service_->GetPlaylistItem(id);
  // Set only for streams, and what tells the front end to play the manifest.
  ASSERT_TRUE(cached->hls_media_path.is_valid());
  EXPECT_TRUE(cached->hls_media_path.spec().ends_with("master.m3u8"));

  auto* web_contents = NavigateToPlayerWebUI();
  auto played = content::EvalJs(
      web_contents,
      content::JsReplace(kPlayScript, GetManifestURL(id, "master.m3u8")));
  ASSERT_TRUE(played.is_ok()) << played.ExtractError();

  const base::DictValue& observed = played.ExtractDict();
  EXPECT_EQ(1280, *observed.FindInt("videoWidth"));
  EXPECT_GT(*observed.FindDouble("audioBytes"), 0.0);
}

// DASH end to end. The MPD keeps audio and video in separate adaptation sets,
// so this is the case that nothing in the tree could mux: it only plays
// because the stream is repackaged as a local HLS multivariant manifest.
IN_PROC_BROWSER_TEST_F(PlaylistStreamPlaybackBrowserTest,
                       DownloadsAndPlaysDashStream) {
  auto [id, dir] = CreateEmptyStreamItem();

  auto result =
      DownloadStream(https_server_->GetURL("/playlist/dash/manifest.mpd"), dir);
  ASSERT_TRUE(result.has_value())
      << PlaylistStreamDownloader::ErrorToString(result.error());
  EXPECT_EQ("master.m3u8", result->manifest_file_name);

  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("master.m3u8")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("video.m3u8")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("audio.m3u8")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("v_init.mp4")));
    EXPECT_TRUE(base::PathExists(dir.AppendASCII("a_00002.m4s")));
  }

  auto* web_contents = NavigateToPlayerWebUI();
  auto played = content::EvalJs(
      web_contents,
      content::JsReplace(kPlayScript,
                         GetManifestURL(id, result->manifest_file_name)));
  ASSERT_TRUE(played.is_ok()) << played.ExtractError();

  const base::DictValue& observed = played.ExtractDict();
  EXPECT_EQ(1280, *observed.FindInt("videoWidth"));
  EXPECT_GT(*observed.FindDouble("videoBytes"), 0.0);
  EXPECT_GT(*observed.FindDouble("audioBytes"), 0.0);
}

}  // namespace playlist
