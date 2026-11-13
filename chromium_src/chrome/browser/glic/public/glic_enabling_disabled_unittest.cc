/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/glic/public/glic_enabling.h"
#include "chrome/common/chrome_features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace glic {

class BraveGlicEnablingTest : public testing::Test {
 public:
  void SetUp() override {
    // Everything upstream requires for Glic is in place, so only Brave's
    // override can turn it off.
    feature_list_.InitWithFeatures({features::kGlic},
                                   {features::kGlicLocaleFiltering});
    GlicEnabling::SetSystemRequirementMetForTesting(true);
  }

  void TearDown() override {
    GlicEnabling::SetSystemRequirementMetForTesting(std::nullopt);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveGlicEnablingTest, GlobalCriteriaAreNeverMet) {
  EXPECT_FALSE(GlicGlobalEnabling().IsEnabledByGlobalCriteria());
}

TEST_F(BraveGlicEnablingTest, BypassingEnablementChecksStillEnablesGlic) {
  GlicEnabling::ScopedBypassEnablementChecksForTesting bypass;
  EXPECT_TRUE(GlicGlobalEnabling().IsEnabledByGlobalCriteria());
}

}  // namespace glic
