/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/media_detector_component_manager.h"

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {

////////////////////////////////////////////////////////////////////////////////
// MediaDetectorComponentManagerTest fixture
class MediaDetectorComponentManagerTest : public testing::Test {
 public:
  // testing::Test:
  void SetUp() override {
    manager_ =
        std::make_unique<playlist::MediaDetectorComponentManager>(nullptr);
  }

  playlist::MediaDetectorComponentManager& manager() { return *manager_; }

 private:
  std::unique_ptr<playlist::MediaDetectorComponentManager> manager_;
};

TEST_F(MediaDetectorComponentManagerTest, SitesThatNeedsURLRuleForMediaPage) {
  // When running a media detection script on background web contents to hide
  // the MediaSrc API or fake the UA string, url rules are required. These rules
  // indicate that a page could potentially contain media. This approach is
  // necessary to avoid performing the expensive task of running the media
  // detection script every time navigation occurs. Therefore, for these
  // specific sites, we assume that a page contains media if its URL matches the
  // defined rule.

  base::flat_set<net::SchemefulSite> sites;
  base::ranges::copy(manager().sites_to_hide_media_src_api_,
                     std::inserter(sites, sites.end()));
  base::ranges::copy(manager().sites_to_use_fake_ua_,
                     std::inserter(sites, sites.end()));

  for (auto& site : sites) {
    EXPECT_TRUE(
        base::Contains(manager().site_and_media_page_url_checkers_, site))
        << "A media page url rule for " << site << " should exist";
  }
}

TEST_F(MediaDetectorComponentManagerTest, YoutubeMediaURL) {
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://www.youtube.com/")));
  EXPECT_FALSE(manager().CouldURLHaveMedia(
      GURL("https://www.youtube.com/@BraveSoftware")));
  EXPECT_FALSE(manager().CouldURLHaveMedia(
      GURL("https://www.youtube.com/feed/history")));
  EXPECT_FALSE(manager().CouldURLHaveMedia(
      GURL("https://www.youtube.com/playlist?list=WL")));
  EXPECT_FALSE(
      manager().CouldURLHaveMedia(GURL("https://www.youtube.com/watch")));
  EXPECT_TRUE(manager().CouldURLHaveMedia(
      GURL("https://www.youtube.com/watch?v=rxtWTT9Jxnc")));
  EXPECT_TRUE(manager().CouldURLHaveMedia(GURL(
      "https://www.youtube.com/watch?v=1231231&list=abcde3&start_radio=1")));
}

TEST_F(MediaDetectorComponentManagerTest, BBCGoodFoodMediaURL) {
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://bbcgoodfood.com/")));
  EXPECT_FALSE(
      manager().CouldURLHaveMedia(GURL("https://bbcgoodfood.com/recipes/foo")));
  EXPECT_FALSE(
      manager().CouldURLHaveMedia(GURL("https://bbcgoodfood.com/videos")));
  EXPECT_TRUE(
      manager().CouldURLHaveMedia(GURL("https://bbcgoodfood.com/videos/foo")));
}

TEST_F(MediaDetectorComponentManagerTest, BitchuteMediaURL) {
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://bitchute.com/")));
  EXPECT_FALSE(manager().CouldURLHaveMedia(
      GURL("https://www.bitchute.com/channel/foo/")));
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://bitchute.com/video")));
  EXPECT_TRUE(
      manager().CouldURLHaveMedia(GURL("https://bitchute.com/video/foo")));
}

TEST_F(MediaDetectorComponentManagerTest, TEDMediaURL) {
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://ted.com/")));
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://www.ted.com/talks/")));
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL(
      "https://www.ted.com/playlists/839/the_most_popular_ted_talks_of_2023")));
  EXPECT_TRUE(manager().CouldURLHaveMedia(
      GURL("https://www.ted.com/talks/foo_bar_baz")));
}

TEST_F(MediaDetectorComponentManagerTest, BrighteonMediaURL) {
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://brighteon.com")));
  EXPECT_FALSE(
      manager().CouldURLHaveMedia(GURL("https://brighteon.com/foo/bar")));
  EXPECT_FALSE(
      manager().CouldURLHaveMedia(GURL("https://brighteon.com/1-2-3-4")));
  EXPECT_TRUE(manager().CouldURLHaveMedia(
      GURL("https://brighteon.com/XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX")));
}

TEST_F(MediaDetectorComponentManagerTest, RumbleMediaURL) {
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://rumble.com")));
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://rumble.com/foo/bar")));
  EXPECT_FALSE(manager().CouldURLHaveMedia(GURL("https://rumble.com/v1")));
  EXPECT_TRUE(manager().CouldURLHaveMedia(
      GURL("https://rumble.com/v123456-abc-def.html")));
}

}  // namespace playlist
