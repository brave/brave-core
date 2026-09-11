/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

BASE_OVERRIDDEN_FEATURE(kCompileOverriddenFeatureForTesting,
                        FEATURE_ENABLED_BY_DEFAULT);
BASE_OVERRIDDEN_FEATURE(kExplicitCompileOverriddenFeatureForTesting,
                        "ExplicitCompileOverriddenFeatureForTesting",
                        FEATURE_DISABLED_BY_DEFAULT);

TEST(CompileOverriddenFeaturesTest, IsFeatureOverridden) {
  FeatureList feature_list;
  EXPECT_TRUE(feature_list.IsFeatureOverridden(
      kCompileOverriddenFeatureForTesting.name));
  EXPECT_TRUE(feature_list.IsFeatureOverridden(
      kExplicitCompileOverriddenFeatureForTesting.name));
  EXPECT_FALSE(feature_list.IsFeatureOverriddenFromCommandLine(
      kCompileOverriddenFeatureForTesting.name));
  EXPECT_FALSE(feature_list.IsFeatureOverriddenFromCommandLine(
      kExplicitCompileOverriddenFeatureForTesting.name));
  EXPECT_EQ(kCompileOverriddenFeatureForTesting.default_state,
            FEATURE_ENABLED_BY_DEFAULT);
  EXPECT_EQ(kExplicitCompileOverriddenFeatureForTesting.default_state,
            FEATURE_DISABLED_BY_DEFAULT);
}

TEST(CompileOverriddenFeaturesTest, GetStateIfOverridden) {
  test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatureList(std::make_unique<FeatureList>());

  EXPECT_EQ(
      FeatureList::GetStateIfOverridden(kCompileOverriddenFeatureForTesting),
      true);
  EXPECT_EQ(FeatureList::GetStateIfOverridden(
                kExplicitCompileOverriddenFeatureForTesting),
            false);
}

}  // namespace
}  // namespace base
