/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/de_amp/browser/de_amp_service.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/de_amp/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace de_amp {

class DeAmpPrefToggleUnitTest : public testing::Test {
 public:
  DeAmpPrefToggleUnitTest() {}
  ~DeAmpPrefToggleUnitTest() override = default;

  void SetUp() override {
    pref_service_ = std::make_unique<TestingPrefServiceSimple>();
    DeAmpService::RegisterProfilePrefs(pref_service_->registry());
    service_ = std::make_unique<DeAmpService>(pref_service_.get());
    scoped_feature_list_.InitAndEnableFeature(features::kBraveDeAMP);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<de_amp::DeAmpService> service_;
};

TEST_F(DeAmpPrefToggleUnitTest, CheckTogglePref) {
  pref_service_->SetBoolean(kDeAmpPrefEnabled, false);
  service_->ToggleDeAmp(true);
  EXPECT_TRUE(pref_service_->GetBoolean(kDeAmpPrefEnabled));
  service_->ToggleDeAmp(false);
  EXPECT_FALSE(pref_service_->GetBoolean(kDeAmpPrefEnabled));
}

TEST_F(DeAmpPrefToggleUnitTest, CheckIsEnabled) {
  pref_service_->SetBoolean(kDeAmpPrefEnabled, false);
  EXPECT_FALSE(service_->IsEnabled());
  pref_service_->SetBoolean(kDeAmpPrefEnabled, true);
  EXPECT_TRUE(service_->IsEnabled());
}

}  // namespace de_amp
