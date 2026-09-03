/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_constants.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {

TEST(PlaylistConstantsTest, IdentifiesYouTubeLegacySites) {
  EXPECT_TRUE(IsYoutubeLegacyPlaylistSite(GURL("https://youtube.com/watch")));
  EXPECT_TRUE(
      IsYoutubeLegacyPlaylistSite(GURL("https://www.youtube.com/watch")));
  EXPECT_TRUE(
      IsYoutubeLegacyPlaylistSite(GURL("https://music.youtube.com/watch")));
  EXPECT_FALSE(IsYoutubeLegacyPlaylistSite(GURL("http://youtube.com/watch")));
  EXPECT_FALSE(
      IsYoutubeLegacyPlaylistSite(GURL("https://youtube-nocookie.com/embed")));
  EXPECT_FALSE(
      IsYoutubeLegacyPlaylistSite(GURL("https://r1.googlevideo.com/video")));
}

}  // namespace playlist
