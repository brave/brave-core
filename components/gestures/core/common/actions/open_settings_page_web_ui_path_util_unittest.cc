// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/open_settings_page_web_ui_path_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(OpenSettingsPageWebUIPathUtilTest,
     ShouldReturnEmptySubpagePathForBasicSettings) {
  EXPECT_THAT(ToSettingsSubPage(OpenSettingsPageType::kBasic),
              testing::IsEmpty());
}

TEST(OpenSettingsPageWebUIPathUtilTest, ShouldReturnMappedSubpagePath) {
  EXPECT_EQ("privacy", ToSettingsSubPage(OpenSettingsPageType::kPrivacy));
  EXPECT_EQ("system/gestures",
            ToSettingsSubPage(OpenSettingsPageType::kGestures));
}

TEST(OpenSettingsPageWebUIPathUtilTest,
     ShouldReturnEmptySubpagePathForOutOfRangeValue) {
  EXPECT_THAT(ToSettingsSubPage(static_cast<OpenSettingsPageType>(999999)),
              testing::IsEmpty());
}

}  // namespace gestures
