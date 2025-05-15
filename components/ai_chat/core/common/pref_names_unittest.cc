// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/pref_names.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat::prefs {

TEST(AIChatPrefNamesTest,
     PageContextEnabledInitiallyIsEnabledWhenFlagIsEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kPageContextEnabledInitially);

  TestingPrefServiceSimple pref_service;
  RegisterProfilePrefs(pref_service.registry());

  EXPECT_TRUE(IsPageContextEnabledInitially(pref_service));
}

TEST(AIChatPrefNamesTest,
     PageContextEnabledInitiallyIsDisabledWhenFlagIsDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kPageContextEnabledInitially);

  TestingPrefServiceSimple pref_service;
  RegisterProfilePrefs(pref_service.registry());

  EXPECT_FALSE(IsPageContextEnabledInitially(pref_service));
}

TEST(AIChatPrefNamesTest, PageContextEnabledInitiallyEnabledOverridesFlag) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kPageContextEnabledInitially);

  TestingPrefServiceSimple pref_service;
  RegisterProfilePrefs(pref_service.registry());
  pref_service.SetBoolean(kBraveChatPageContextEnabledInitially, true);

  EXPECT_TRUE(IsPageContextEnabledInitially(pref_service));
}

TEST(AIChatPrefNamesTest, PageContextEnabledInitiallyDisabledOverridesFlag) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kPageContextEnabledInitially);

  TestingPrefServiceSimple pref_service;
  RegisterProfilePrefs(pref_service.registry());
  pref_service.SetBoolean(kBraveChatPageContextEnabledInitially, false);

  EXPECT_FALSE(IsPageContextEnabledInitially(pref_service));
}

}  // namespace ai_chat::prefs
