/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/test/mock_callback.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

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

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kTestEnabledButOverridenFeature, FEATURE_DISABLED_BY_DEFAULT},
    {kTestDisabledButOverridenFeature, FEATURE_ENABLED_BY_DEFAULT},
    {kTestConstexprEnabledButOverridenFeature, FEATURE_DISABLED_BY_DEFAULT},
    {kTestConstexprDisabledButOverridenFeature, FEATURE_ENABLED_BY_DEFAULT},
}});

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

#if DCHECK_IS_ON()
TEST(FeatureOverrideTest, FeatureDuplicateDChecks) {
  base::MockCallback<logging::LogAssertHandlerFunction> mock_log_handler;
  logging::ScopedLogAssertHandler scoped_log_handler(mock_log_handler.Get());
  EXPECT_CALL(
      mock_log_handler,
      Run(_, _,
          testing::HasSubstr("TestEnabledButOverridenFeature is duplicated"),
          _));
  EXPECT_CALL(
      mock_log_handler,
      Run(_, _,
          testing::HasSubstr(
              "TestEnabledButOverridenFeature has already been overridden"),
          _))
      .Times(2);

  internal::FeatureDefaultStateOverrider test_overrider{{
      {kTestEnabledButOverridenFeature, FEATURE_DISABLED_BY_DEFAULT},
      {kTestEnabledButOverridenFeature, FEATURE_DISABLED_BY_DEFAULT},
  }};
}
#endif  // DCHECK_IS_ON()

}  // namespace base
