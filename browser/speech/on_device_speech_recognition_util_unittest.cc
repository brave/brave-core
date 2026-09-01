/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/files/file_path.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/prefs/pref_service.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speech {

media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality);

namespace {

using AvailabilityStatus = media::mojom::AvailabilityStatus;
using Quality = media::mojom::SpeechRecognitionQuality;

}  // namespace

class BraveOnDeviceSpeechAvailabilityUnitTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        local_ai::kBraveOnDeviceSpeechRecognition);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, DownloadableUntilInstalled) {
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability("en-US", Quality::kCommand));

  state->SetInstallDir(
      base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  EXPECT_EQ(AvailabilityStatus::kAvailable,
            GetBraveOnDeviceSpeechAvailability("en-US", Quality::kCommand));

  state->SetInstallDir(base::FilePath());
}

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, UnavailableWhenLocalAIOff) {
  PrefService* local_state = TestingBrowserProcess::GetGlobal()->local_state();
  local_state->SetBoolean(local_ai::prefs::kBraveLocalAIEnabled, false);
  EXPECT_EQ(AvailabilityStatus::kUnavailable,
            GetBraveOnDeviceSpeechAvailability("en-US", Quality::kCommand));
  local_state->ClearPref(local_ai::prefs::kBraveLocalAIEnabled);
}

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, UnavailableWhenFeatureOff) {
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(local_ai::kBraveOnDeviceSpeechRecognition);
  EXPECT_EQ(AvailabilityStatus::kUnavailable,
            GetBraveOnDeviceSpeechAvailability("en-US", Quality::kCommand));
}

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, ServesCommandAndDictation) {
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability("en-US", Quality::kCommand));
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability("en-US", Quality::kDictation));
  EXPECT_EQ(
      AvailabilityStatus::kUnavailable,
      GetBraveOnDeviceSpeechAvailability("en-US", Quality::kConversation));
}

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, EnglishLocalesOnly) {
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability("en-US", Quality::kCommand));
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability("EN-GB", Quality::kCommand));
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability("en", Quality::kCommand));
  EXPECT_EQ(AvailabilityStatus::kUnavailable,
            GetBraveOnDeviceSpeechAvailability("fr-FR", Quality::kCommand));
  EXPECT_EQ(AvailabilityStatus::kUnavailable,
            GetBraveOnDeviceSpeechAvailability("", Quality::kCommand));
}

}  // namespace speech
