/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/common/feature_switch.h"

#include "base/command_line.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveFeatureSwitchTest, LoadMediaRouterComponentExtensionFlagTest) {
  EXPECT_TRUE(extensions::FeatureSwitch::load_media_router_component_extension()
      ->IsEnabled());
}
