/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "base/feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

const Feature kTestControlEnabledFeature{"TestControlEnabledFeature",
                                         FEATURE_ENABLED_BY_DEFAULT};
const Feature kTestControlDisabledFeature{"TestControlDisabledFeature",
                                          FEATURE_DISABLED_BY_DEFAULT};

const Feature kTestEnabledButOverridenFeature{"TestEnabledButOverridenFeature",
                                              FEATURE_DISABLED_BY_DEFAULT};
const Feature kTestDisabledButOverridenFeature{
    "TestDisabledButOverridenFeature", FEATURE_ENABLED_BY_DEFAULT};

constexpr Feature kTestConstexprEnabledButOverridenFeature{
    "TestConstexprEnabledButOverridenFeature", FEATURE_DISABLED_BY_DEFAULT};
constexpr Feature kTestConstexprDisabledButOverridenFeature{
    "TestConstexprDisabledButOverridenFeature", FEATURE_ENABLED_BY_DEFAULT};

ENABLE_FEATURE_BY_DEFAULT(kTestDisabledButOverridenFeature);
ENABLE_FEATURE_BY_DEFAULT(kTestConstexprDisabledButOverridenFeature);
DISABLE_FEATURE_BY_DEFAULT(kTestEnabledButOverridenFeature);
DISABLE_FEATURE_BY_DEFAULT(kTestConstexprEnabledButOverridenFeature);

}  // namespace

TEST(FeatureOverrideTest, OverridesTest) {
  struct TestCase {
    const Feature& feature;
    const bool is_enabled;
  };
  const TestCase kTestCases[] = {
      // Untouched features.
      {kTestControlEnabledFeature, true},
      {kTestControlDisabledFeature, false},

      // Overridden features.
      {kTestEnabledButOverridenFeature, false},
      {kTestDisabledButOverridenFeature, true},

      // Overridden constexpr features.
      {kTestConstexprEnabledButOverridenFeature, false},
      {kTestConstexprDisabledButOverridenFeature, true},
  };
  for (const auto& test_case : kTestCases) {
    EXPECT_EQ(test_case.is_enabled, FeatureList::IsEnabled(test_case.feature))
        << test_case.feature.name;
  }
}

}  // namespace base
