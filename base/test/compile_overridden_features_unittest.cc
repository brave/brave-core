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

BASE_FEATURE(kNonCompileOverriddenFeatureForTesting,
             FEATURE_ENABLED_BY_DEFAULT);
BASE_OVERRIDDEN_FEATURE(kCompileOverriddenFeatureForTesting,
                        FEATURE_ENABLED_BY_DEFAULT);
BASE_OVERRIDDEN_FEATURE(kExplicitCompileOverriddenFeatureForTesting,
                        "ExplicitCompileOverriddenFeatureForTesting",
                        FEATURE_DISABLED_BY_DEFAULT);

}  // namespace

class CompileOverriddenFeaturesTest : public testing::Test {
 protected:
  void SetUp() override {
    scoped_feature_list_.InitWithFeatureList(std::make_unique<FeatureList>());
  }

 private:
  test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(CompileOverriddenFeaturesTest, IsFeatureOverridden) {
  EXPECT_FALSE(base::FeatureList::GetInstance()->IsFeatureOverridden(
      kNonCompileOverriddenFeatureForTesting.name));
  EXPECT_TRUE(base::FeatureList::GetInstance()->IsFeatureOverridden(
      kCompileOverriddenFeatureForTesting.name));
  EXPECT_TRUE(base::FeatureList::GetInstance()->IsFeatureOverridden(
      kExplicitCompileOverriddenFeatureForTesting.name));

  EXPECT_FALSE(
      base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
          kNonCompileOverriddenFeatureForTesting.name));
  EXPECT_FALSE(
      base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
          kCompileOverriddenFeatureForTesting.name));
  EXPECT_FALSE(
      base::FeatureList::GetInstance()->IsFeatureOverriddenFromCommandLine(
          kExplicitCompileOverriddenFeatureForTesting.name));

  EXPECT_EQ(kNonCompileOverriddenFeatureForTesting.default_state,
            FEATURE_ENABLED_BY_DEFAULT);
  EXPECT_EQ(kCompileOverriddenFeatureForTesting.default_state,
            FEATURE_ENABLED_BY_DEFAULT);
  EXPECT_EQ(kExplicitCompileOverriddenFeatureForTesting.default_state,
            FEATURE_DISABLED_BY_DEFAULT);
}

TEST_F(CompileOverriddenFeaturesTest, GetStateIfOverridden) {
  EXPECT_EQ(
      FeatureList::GetStateIfOverridden(kNonCompileOverriddenFeatureForTesting),
      std::nullopt);
  EXPECT_EQ(
      FeatureList::GetStateIfOverridden(kCompileOverriddenFeatureForTesting),
      true);
  EXPECT_EQ(FeatureList::GetStateIfOverridden(
                kExplicitCompileOverriddenFeatureForTesting),
            false);
}

}  // namespace base
