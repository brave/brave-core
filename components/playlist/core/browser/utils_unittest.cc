/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/core/browser/utils.h"

#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/playlist/core/common/features.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace playlist {

class PlaylistUtilsTest : public testing::Test {
 public:
  PlaylistUtilsTest() {
    prefs_.registry()->RegisterBooleanPref(kPlaylistEnabledPref, true);
  }

  TestingPrefServiceSimple prefs_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(PlaylistUtilsTest, DisallowedWhenFeatureOff) {
  feature_list_.InitAndDisableFeature(features::kPlaylist);
  EXPECT_FALSE(IsPlaylistAllowed(&prefs_));
}

TEST_F(PlaylistUtilsTest, AllowedByDefaultWithFeatureOn) {
  feature_list_.InitAndEnableFeature(features::kPlaylist);
  EXPECT_TRUE(IsPlaylistAllowed(&prefs_));
}

TEST_F(PlaylistUtilsTest, UserPrefOffKeepsPlaylistAllowed) {
  feature_list_.InitAndEnableFeature(features::kPlaylist);
  // A user turning the pref off from settings is not a policy-managed state:
  // the UI must remain reachable so the user can flip the pref back.
  prefs_.SetUserPref(kPlaylistEnabledPref, base::Value(false));
  EXPECT_TRUE(IsPlaylistAllowed(&prefs_));
}

TEST_F(PlaylistUtilsTest, ManagedPrefForcedOffDisallows) {
  feature_list_.InitAndEnableFeature(features::kPlaylist);
  prefs_.SetManagedPref(kPlaylistEnabledPref, base::Value(false));
  EXPECT_FALSE(IsPlaylistAllowed(&prefs_));
}

TEST_F(PlaylistUtilsTest, ManagedPrefForcedOnKeepsAllowed) {
  feature_list_.InitAndEnableFeature(features::kPlaylist);
  prefs_.SetManagedPref(kPlaylistEnabledPref, base::Value(true));
  EXPECT_TRUE(IsPlaylistAllowed(&prefs_));
}

}  // namespace playlist
