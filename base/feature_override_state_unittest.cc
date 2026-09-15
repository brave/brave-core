/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/feature.h"
#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

BASE_FEATURE(kDisabledByOverride, FEATURE_DISABLED_BY_OVERRIDE);
BASE_FEATURE(kDisabledByDefault, FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kEnabledByDefault, FEATURE_ENABLED_BY_DEFAULT);

static_assert(!IsCountrySpecificFeatureState(FEATURE_DISABLED_BY_OVERRIDE));

}  // namespace

TEST(FeatureOverrideStateTest, FeatureIsDisabled) {
  EXPECT_FALSE(FeatureList::IsEnabled(kDisabledByOverride));
}

TEST(FeatureOverrideStateTest, StateIsReportedAsAnOverride) {
  EXPECT_EQ(FeatureList::GetStateIfOverridden(kDisabledByOverride), false);
}

TEST(FeatureOverrideStateTest, DefaultStatesAreNotReportedAsOverrides) {
  EXPECT_FALSE(FeatureList::IsEnabled(kDisabledByDefault));
  EXPECT_EQ(FeatureList::GetStateIfOverridden(kDisabledByDefault),
            std::nullopt);

  EXPECT_TRUE(FeatureList::IsEnabled(kEnabledByDefault));
  EXPECT_EQ(FeatureList::GetStateIfOverridden(kEnabledByDefault), std::nullopt);
}

TEST(FeatureOverrideStateTest, RealOverridesTakePrecedence) {
  test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kDisabledByOverride);

  EXPECT_TRUE(FeatureList::IsEnabled(kDisabledByOverride));
  EXPECT_EQ(FeatureList::GetStateIfOverridden(kDisabledByOverride), true);
}

}  // namespace base
