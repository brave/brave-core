// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/switch_to_tab_params_util.h"

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(SwitchToTabParamsUtilTest,
     ParseSwitchToTabParamsReturnsAnEmptyPatternWhenFieldIsMissing) {
  const base::DictValue action_dict;

  EXPECT_EQ("", ParseSwitchToTabParams(action_dict).pattern);
}

TEST(SwitchToTabParamsUtilTest,
     ParseSwitchToTabParamsReturnsAnEmptyPatternWhenFieldIsNotAString) {
  base::DictValue action_dict;
  action_dict.Set("switch_to_tab_pattern", 42);

  EXPECT_EQ("", ParseSwitchToTabParams(action_dict).pattern);
}

TEST(SwitchToTabParamsUtilTest, ParseSwitchToTabParamsReturnsTheStoredPattern) {
  base::DictValue action_dict;
  action_dict.Set("switch_to_tab_pattern", "*gmail*");

  EXPECT_EQ("*gmail*", ParseSwitchToTabParams(action_dict).pattern);
}

}  // namespace gestures
