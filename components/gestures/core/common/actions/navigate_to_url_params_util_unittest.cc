// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/navigate_to_url_params_util.h"

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(NavigateToUrlParamsUtilTest,
     ParseNavigateToUrlParamsReturnsAnEmptyUrlWhenFieldIsMissing) {
  const base::DictValue action_dict;

  EXPECT_EQ("", ParseNavigateToUrlParams(action_dict).url_spec);
}

TEST(NavigateToUrlParamsUtilTest,
     ParseNavigateToUrlParamsReturnsAnEmptyUrlWhenFieldIsNotAString) {
  base::DictValue action_dict;
  action_dict.Set("navigate_to_url", 42);

  EXPECT_EQ("", ParseNavigateToUrlParams(action_dict).url_spec);
}

TEST(NavigateToUrlParamsUtilTest, ParseNavigateToUrlParamsReturnsTheStoredUrl) {
  base::DictValue action_dict;
  action_dict.Set("navigate_to_url", "https://brave.com");

  EXPECT_EQ("https://brave.com",
            ParseNavigateToUrlParams(action_dict).url_spec);
}

}  // namespace gestures
