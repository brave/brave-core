/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <limits.h>

#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/updater/features.h"
#include "chrome/browser/ui/webui/help/version_updater.h"
#include "chrome/browser/ui/webui/help/version_updater_mac.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveVersionUpdaterMacTest : public testing::Test {
 protected:
  bool UsesSparkle() {
    std::unique_ptr<VersionUpdater> updater = VersionUpdater::Create(nullptr);
    bool result = false;
    updater->GetIsSparkleForTesting(result);
    return result;
  }
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(BraveVersionUpdaterMacTest, UsesSparkleWhenFeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(brave_updater::kBraveUseOmaha4);
  EXPECT_TRUE(UsesSparkle());
}

TEST_F(BraveVersionUpdaterMacTest, UsesOmaha4WhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      brave_updater::kBraveUseOmaha4,
      {{brave_updater::kLegacyFallbackIntervalDays.name,
        base::NumberToString(INT_MAX)}});
  EXPECT_FALSE(UsesSparkle());
}
