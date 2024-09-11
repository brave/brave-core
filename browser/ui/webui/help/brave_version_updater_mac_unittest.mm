/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/mac_features.h"
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

TEST_F(BraveVersionUpdaterMacTest, UsesSparkleByDefault) {
  EXPECT_TRUE(UsesSparkle());
}

TEST_F(BraveVersionUpdaterMacTest, UsesOmaha4WhenEnabled) {
  scoped_feature_list_.InitAndEnableFeature(brave::kBraveUseOmaha4Alpha);
  EXPECT_FALSE(UsesSparkle());
}
