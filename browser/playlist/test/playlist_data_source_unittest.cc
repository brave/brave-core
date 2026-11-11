// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/playlist/playlist_data_source.h"

#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace playlist {

TEST(PlaylistDataSourceTest, AllowsOnlyPlaylistOrigins) {
  content::BrowserTaskEnvironment task_environment;
  TestingProfile profile;
  PlaylistDataSource source(&profile, /*service=*/nullptr);

  for (const char* origin : {
           "chrome-untrusted://playlist",
           "chrome-untrusted://playlist-player",
       }) {
    SCOPED_TRACE(origin);
    EXPECT_EQ(source.GetAccessControlAllowOriginForOrigin(origin), origin);
  }

  for (const char* origin : {
           "chrome-untrusted://leo-ai-conversation-entries",
           "chrome-untrusted://data-sharing",
           "https://example.com",
       }) {
    SCOPED_TRACE(origin);
    EXPECT_EQ(source.GetAccessControlAllowOriginForOrigin(origin), "");
  }
}

}  // namespace playlist
