/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/test/mock_callback.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace base {
namespace {

BASE_FEATURE(kTestControlEnabledFeature,
             "TestControlEnabledFeature",
             FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE(kTestControlDisabledFeature,
             "TestControlDisabledFeature",
             FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kTestEnabledButOverridenFeature,
             "TestEnabledButOverridenFeature",
             FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE(kTestDisabledButOverridenFeature,
             "TestDisabledButOverridenFeature",
             FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kTestEnabledButOverridenFeatureWithSameState,
             "TestEnabledButOverridenFeatureWithSameState",
             FEATURE_ENABLED_BY_DEFAULT);

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kTestEnabledButOverridenFeature, FEATURE_DISABLED_BY_DEFAULT},
    {kTestDisabledButOverridenFeature, FEATURE_ENABLED_BY_DEFAULT},

    // Override, but keep the same state as `default_state`. We should properly
    // return false from IsFeatureOverridden in this case.
    {kTestEnabledButOverridenFeatureWithSameState, FEATURE_ENABLED_BY_DEFAULT},
}});

}  // namespace

TEST(FeatureOverrideTest, OverridesTest) {
  struct TestCase {
    const Feature& feature;
    const bool is_enabled;
    const bool is_overridden;
  };
  constexpr TestCase kTestCases[] = {
      // Untouched features.
      {kTestControlEnabledFeature, true, false},
      {kTestControlDisabledFeature, false, false},

      // Overridden features.
      {kTestEnabledButOverridenFeature, false, true},
      {kTestDisabledButOverridenFeature, true, true},

      // Overridden but with the same state.
      {kTestEnabledButOverridenFeatureWithSameState, true, false},
  };
  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message() << test_case.feature.name);
    EXPECT_EQ(test_case.is_enabled, FeatureList::IsEnabled(test_case.feature));
    EXPECT_EQ(test_case.is_overridden,
              FeatureList::GetInstance()->IsFeatureOverridden(
                  test_case.feature.name));
  }
}

#if DCHECK_IS_ON()
TEST(FeatureOverrideTest, FeatureDuplicateDChecks) {
  // Check any feature to make sure overridden features are finalized (moved
  // from an unsorted vector to a sorted flat_map).
  ASSERT_FALSE(base::FeatureList::IsEnabled(kTestEnabledButOverridenFeature));

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

  // This will add a feature to an unsorted vector of overrides.
  internal::FeatureDefaultStateOverrider init_overrides{{
      {kTestEnabledButOverridenFeature, FEATURE_DISABLED_BY_DEFAULT},
  }};

  // This should trigger DCHECKs.
  internal::FeatureDefaultStateOverrider test_overrider{{
      {kTestEnabledButOverridenFeature, FEATURE_DISABLED_BY_DEFAULT},
      {kTestEnabledButOverridenFeature, FEATURE_DISABLED_BY_DEFAULT},
  }};
}
#endif  // DCHECK_IS_ON()

}  // namespace base
