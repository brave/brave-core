/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_features.h"

#include <string>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace features {

namespace {

FocusModeUrlDisplay GetUrlDisplayFor(const std::string& param_value) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      kBraveFocusMode, {{"url-display", param_value}});
  return GetFocusModeUrlDisplay();
}

}  // namespace

TEST(FocusModeFeaturesTest, UrlDisplayDefaultsToTitleBar) {
  EXPECT_EQ(GetFocusModeUrlDisplay(), FocusModeUrlDisplay::kTitleBar);

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kBraveFocusMode);
  EXPECT_EQ(GetFocusModeUrlDisplay(), FocusModeUrlDisplay::kTitleBar);
}

TEST(FocusModeFeaturesTest, UrlDisplayFromFeatureParam) {
  // These values must match the variations registered in about_flags.cc.
  EXPECT_EQ(GetUrlDisplayFor("none"), FocusModeUrlDisplay::kNone);
  EXPECT_EQ(GetUrlDisplayFor("title-bar"), FocusModeUrlDisplay::kTitleBar);
  EXPECT_EQ(GetUrlDisplayFor("mini-toolbar"),
            FocusModeUrlDisplay::kMiniToolbar);

  EXPECT_EQ(GetUrlDisplayFor("bogus"), FocusModeUrlDisplay::kTitleBar);
}

}  // namespace features
