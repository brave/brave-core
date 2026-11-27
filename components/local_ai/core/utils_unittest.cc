/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/utils.h"

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

namespace {

constexpr OnDeviceSpeechModel kEnglish = OnDeviceSpeechModel::kEnglish;
constexpr OnDeviceSpeechModel kMultilingual =
    OnDeviceSpeechModel::kMultilingual;

}  // namespace

class OnDeviceSpeechUtilsUnitTest : public testing::Test {
 public:
  OnDeviceSpeechUtilsUnitTest() {
    feature_list_.InitWithFeatures(
        {kBraveOnDeviceSpeechRecognition, kBraveOnDeviceSpeechMultilingual},
        {});
    prefs::RegisterLocalStatePrefs(local_state_.registry());
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
};

// The pref holds at most one model. Enabling a model replaces the one before.
TEST_F(OnDeviceSpeechUtilsUnitTest, EnablingAModelReplacesTheOneBefore) {
  EXPECT_EQ(std::nullopt, GetEnabledOnDeviceSpeechModel(&local_state_));

  EnableOnDeviceSpeechModel(local_state_, kEnglish);
  EXPECT_EQ(kEnglish, GetEnabledOnDeviceSpeechModel(&local_state_));
  EXPECT_TRUE(ShouldInstallOnDeviceSpeechModel(&local_state_, kEnglish));
  EXPECT_FALSE(ShouldInstallOnDeviceSpeechModel(&local_state_, kMultilingual));

  EnableOnDeviceSpeechModel(local_state_, kMultilingual);
  EXPECT_EQ(kMultilingual, GetEnabledOnDeviceSpeechModel(&local_state_));
  EXPECT_FALSE(ShouldInstallOnDeviceSpeechModel(&local_state_, kEnglish));
  EXPECT_TRUE(ShouldInstallOnDeviceSpeechModel(&local_state_, kMultilingual));
}

// A value that no model has, such as one from a later version, reads as no
// model.
TEST_F(OnDeviceSpeechUtilsUnitTest, UnknownPrefValueEnablesNoModel) {
  local_state_.SetInteger(prefs::kOnDeviceSpeechModel, 3);
  EXPECT_EQ(std::nullopt, GetEnabledOnDeviceSpeechModel(&local_state_));
}

}  // namespace local_ai
