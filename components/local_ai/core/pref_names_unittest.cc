/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/pref_names.h"

#include <optional>

#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom-shared.h"
#include "brave/components/local_ai/core/utils.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

class OnDeviceSpeechPrefMigrationUnitTest : public testing::Test {
 public:
  OnDeviceSpeechPrefMigrationUnitTest() {
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    prefs::RegisterLocalStatePrefsForMigration(local_state_.registry());
  }

 protected:
  // Sets the obsolete pref of the English model, migrates, and returns the
  // model that the user then enabled.
  std::optional<OnDeviceSpeechModel> Migrate(bool english_enabled) {
    local_state_.SetBoolean(kEnglishPref, english_enabled);
    prefs::MigrateObsoleteLocalStatePrefs(&local_state_);
    EXPECT_FALSE(local_state_.HasPrefPath(kEnglishPref));
    return GetEnabledOnDeviceSpeechModel(&local_state_);
  }

  // The obsolete pref of the English model.
  static constexpr char kEnglishPref[] =
      "brave.local_ai.on_device_speech_model_enabled";

  TestingPrefServiceSimple local_state_;
};

// The English model carries over when the user enabled it.
TEST_F(OnDeviceSpeechPrefMigrationUnitTest, CarriesOverTheEnglishModel) {
  EXPECT_EQ(std::nullopt, Migrate(/*english_enabled=*/false));
  EXPECT_EQ(OnDeviceSpeechModel::kEnglish, Migrate(/*english_enabled=*/true));
}

}  // namespace local_ai
