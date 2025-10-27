// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/common/brave_shield_utils.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAdBlockOnlyModeEnabled, false);
  registry->RegisterBooleanPref(
      prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale, false);
}

}  // namespace

TEST(BraveShieldsUtilsTest, IsAdblockOnlyModeFeatureEnabled) {
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(features::kAdblockOnlyMode);
    EXPECT_TRUE(IsAdblockOnlyModeFeatureEnabled());
  }
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndDisableFeature(features::kAdblockOnlyMode);
    EXPECT_FALSE(IsAdblockOnlyModeFeatureEnabled());
  }
}

TEST(BraveShieldsUtilsTest, IsBraveShieldsAdBlockOnlyModeEnabled) {
  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());

  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  EXPECT_TRUE(IsBraveShieldsAdBlockOnlyModeEnabled(&local_state));

  local_state.SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
  EXPECT_FALSE(IsBraveShieldsAdBlockOnlyModeEnabled(&local_state));
}

TEST(BraveShieldsUtilsTest, SetBraveShieldsAdBlockOnlyModeEnabled) {
  TestingPrefServiceSimple local_state;
  RegisterLocalStatePrefs(local_state.registry());

  SetBraveShieldsAdBlockOnlyModeEnabled(&local_state, /*enabled=*/true);
  EXPECT_TRUE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));

  SetBraveShieldsAdBlockOnlyModeEnabled(&local_state, /*enabled=*/false);
  EXPECT_FALSE(local_state.GetBoolean(prefs::kAdBlockOnlyModeEnabled));
}

}  // namespace brave_shields
