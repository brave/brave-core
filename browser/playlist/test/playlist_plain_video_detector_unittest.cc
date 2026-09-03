/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_plain_video_detector.h"

#include <string_view>
#include <vector>

#include "base/functional/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/playlist/test/playlist_unittest_base.h"
#include "brave/components/playlist/content/browser/playlist_network_observer.h"
#include "brave/components/playlist/core/common/features.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {

namespace {

MediaResponseInfo Progressive(std::string_view url) {
  return {.url = GURL(url), .kind = MediaKind::kProgressive};
}

}  // namespace

class PlaylistPlainVideoDetectorTest : public PlaylistUnitTestBase {
 protected:
  void SetUp() override {
    PlaylistUnitTestBase::SetUp();
    NavigateAndCommit(GURL("https://example.com"));
    PlaylistPlainVideoDetector::CreateForWebContents(
        web_contents(),
        base::BindRepeating(&PlaylistPlainVideoDetectorTest::OnMediaDetected,
                            base::Unretained(this)));
  }

  void OnMediaDetected(GURL page_url, std::vector<mojom::PlaylistItemPtr> items) {
    last_page_url_ = page_url;
    last_items_ = std::move(items);
  }

  void Notify(const MediaResponseInfo& info) {
    PlaylistNetworkObserver::GetOrCreate(browser_context())
        ->OnMediaResponse(main_rfh()->GetGlobalFrameToken(), info);
  }

  base::test::ScopedFeatureList v2_feature_{features::kPlaylistServiceV2};
  GURL last_page_url_;
  std::vector<mojom::PlaylistItemPtr> last_items_;
};

TEST_F(PlaylistPlainVideoDetectorTest, NotifiesOnPlainVideoSrc) {
  Notify(Progressive("https://cdn.example.com/movie.mp4"));

  ASSERT_EQ(1u, last_items_.size());
  EXPECT_EQ(GURL("https://example.com"), last_page_url_);
  EXPECT_EQ(GURL("https://cdn.example.com/movie.mp4"),
           last_items_[0]->media_source);
}

TEST_F(PlaylistPlainVideoDetectorTest, IgnoresManifestsAndSegments) {
  Notify({.url = GURL("https://cdn.example.com/master.m3u8"),
         .kind = MediaKind::kHlsManifest});
  Notify({.url = GURL("https://cdn.example.com/init.mp4"),
         .kind = MediaKind::kSegment});

  EXPECT_TRUE(last_items_.empty());
}

TEST_F(PlaylistPlainVideoDetectorTest, DedupesRepeatedNotifications) {
  Notify(Progressive("https://cdn.example.com/movie.mp4"));
  last_items_.clear();

  Notify(Progressive("https://cdn.example.com/movie.mp4"));

  EXPECT_TRUE(last_items_.empty());
}

}  // namespace playlist
