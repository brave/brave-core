/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/core/browser/playlist_exclusion.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace playlist {

class PlaylistExclusionsUnitTest : public testing::Test {
 public:
  PlaylistExclusionsUnitTest() = default;

 protected:
  void SetUp() override { ResetExclusions(PlaylistExclusions::GetInstance()); }

  void ResetExclusions(PlaylistExclusions* exclusions) {
    exclusions->weak_factory_.InvalidateWeakPtrs();
    exclusions->rules_.clear();
    exclusions->is_ready_ = false;
    exclusions->component_path_.clear();
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::UI};
};

TEST_F(PlaylistExclusionsUnitTest, NotReadyIsPermissive) {
  PlaylistExclusions* exclusions = PlaylistExclusions::GetInstance();
  ASSERT_FALSE(exclusions->is_ready_);
  EXPECT_TRUE(exclusions->CanResolvePageSrcLater(GURL("https://youtube.com/")));
}

TEST_F(PlaylistExclusionsUnitTest, EmptyContentsNotReady) {
  PlaylistExclusions* exclusions = PlaylistExclusions::GetInstance();
  exclusions->OnPlaylistExclusionsLoaded("");
  EXPECT_FALSE(exclusions->is_ready_);
}

TEST_F(PlaylistExclusionsUnitTest, InvalidJsonNotReady) {
  PlaylistExclusions* exclusions = PlaylistExclusions::GetInstance();
  exclusions->OnPlaylistExclusionsLoaded("{not valid json");
  EXPECT_FALSE(exclusions->is_ready_);
}

TEST_F(PlaylistExclusionsUnitTest, MissingRulesListNotReady) {
  PlaylistExclusions* exclusions = PlaylistExclusions::GetInstance();
  exclusions->OnPlaylistExclusionsLoaded(R"({"version": 1})");
  EXPECT_FALSE(exclusions->is_ready_);
}

TEST_F(PlaylistExclusionsUnitTest, RulesBlockListedPaths) {
  PlaylistExclusions* exclusions = PlaylistExclusions::GetInstance();

  constexpr char kJson[] = R"({
    "version": 1,
    "rules": [{
      "registrable_domain": "youtube.com",
      "deny_root_path": true,
      "path_prefixes": ["/results", "/feed", "/@"]
    }]
  })";

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  ASSERT_TRUE(base::WriteFile(
      temp_dir.GetPath().AppendASCII("playlist_exclusions.json"), kJson));

  exclusions->OnComponentReady(temp_dir.GetPath());
  ASSERT_TRUE(base::test::RunUntil([&]() { return exclusions->is_ready_; }));

  EXPECT_FALSE(
      exclusions->CanResolvePageSrcLater(GURL("https://youtube.com/")));
  EXPECT_FALSE(
      exclusions->CanResolvePageSrcLater(GURL("https://www.youtube.com")));
  EXPECT_FALSE(exclusions->CanResolvePageSrcLater(
      GURL("https://www.youtube.com/results?foo=1")));
  EXPECT_FALSE(exclusions->CanResolvePageSrcLater(
      GURL("https://www.youtube.com/@Example")));
  EXPECT_TRUE(exclusions->CanResolvePageSrcLater(
      GURL("https://www.youtube.com/watch?v=1")));
}

}  // namespace playlist
