/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "chrome/browser/ui/ui_features.h"

#if BUILDFLAG(IS_MAC)
TEST(FirstRunTest, FeatureTestOnMac) {
  // To detect macOS specific FirstRun dialog deprecation.
  // When it's deprecated, we can delete first_run_dialog_controller.mm
  // overrides.
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kViewsFirstRunDialog));
}
#endif
