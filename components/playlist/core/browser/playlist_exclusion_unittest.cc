/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/playlist/core/browser/playlist_exclusions.h"
#include "brave/components/playlist/core/common/constants.h"
#include "brave/components/playlist/core/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace playlist {

class PlaylistExclusionsUnitTest : public testing::Test {
 public:
  PlaylistExclusionsUnitTest() = default;

 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kPlaylist);
    ResetExclusions(PlaylistExclusions::GetInstance());
  }

  void ResetExclusions(PlaylistExclusions* exclusions) {
    exclusions->weak_factory_.InvalidateWeakPtrs();
    exclusions->rules_.clear();
  }

  void LoadExclusionsAndWait(PlaylistExclusions* exclusions,
                             const base::FilePath& exclusions_file) {
    base::test::TestFuture<void> future;
    exclusions->LoadPlaylistExclusions(exclusions_file, future.GetCallback());
    ASSERT_TRUE(future.Wait());
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::UI};
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(PlaylistExclusionsUnitTest, NotReadyIsPermissive) {
  PlaylistExclusions* exclusions = PlaylistExclusions::GetInstance();
  ASSERT_TRUE(exclusions->rules_.empty());
  EXPECT_TRUE(exclusions->CanResolvePageSrcLater(GURL("https://youtube.com/")));
}

TEST_F(PlaylistExclusionsUnitTest, FailedReloadKeepsLastValidRules) {
  PlaylistExclusions* exclusions = PlaylistExclusions::GetInstance();

  constexpr char kValidJson[] = R"({
    "version": 1,
    "rules": [{
      "registrable_domain": "youtube.com",
      "deny_root_path": true,
      "path_prefixes": ["/results"]
    }]
  })";

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  const base::FilePath valid_file =
      temp_dir.GetPath().AppendASCII("valid_playlist_exclusions.json");
  const base::FilePath invalid_file =
      temp_dir.GetPath().AppendASCII("invalid_playlist_exclusions.json");
  ASSERT_TRUE(base::WriteFile(valid_file, kValidJson));
  ASSERT_TRUE(base::WriteFile(invalid_file, "{not valid json"));

  LoadExclusionsAndWait(exclusions, valid_file);
  ASSERT_FALSE(exclusions->rules_.empty());
  EXPECT_FALSE(
      exclusions->CanResolvePageSrcLater(GURL("https://youtube.com/")));

  LoadExclusionsAndWait(exclusions, invalid_file);
  ASSERT_FALSE(exclusions->rules_.empty());
  EXPECT_FALSE(
      exclusions->CanResolvePageSrcLater(GURL("https://youtube.com/")));
}

TEST_F(PlaylistExclusionsUnitTest, FailedLoadsLeaveRulesEmpty) {
  PlaylistExclusions* exclusions = PlaylistExclusions::GetInstance();

  const struct {
    const char* trace_name;
    const char* contents;
  } kCases[] = {
      {"EmptyContents", ""},
      {"InvalidJson", "{not valid json"},
      {"MissingRulesList", R"({"version": 1})"},
  };

  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.trace_name);

    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
    const base::FilePath exclusions_file =
        temp_dir.GetPath().AppendASCII(kPlaylistExclusionsJsonFile);
    ASSERT_TRUE(base::WriteFile(exclusions_file, test_case.contents));

    LoadExclusionsAndWait(exclusions, exclusions_file);
    EXPECT_TRUE(exclusions->rules_.empty());
  }
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
  const base::FilePath exclusions_file =
      temp_dir.GetPath().AppendASCII(kPlaylistExclusionsJsonFile);
  ASSERT_TRUE(base::WriteFile(exclusions_file, kJson));

  LoadExclusionsAndWait(exclusions, exclusions_file);
  ASSERT_FALSE(exclusions->rules_.empty());

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
  EXPECT_TRUE(
      exclusions->CanResolvePageSrcLater(GURL("https://www.example.com/")));
}

}  // namespace playlist
