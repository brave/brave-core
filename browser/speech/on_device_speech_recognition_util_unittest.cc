/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string_view>
#include <utility>

#include "base/files/file_path.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speech {

media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    content::BrowserContext* context,
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality);

namespace {

using AvailabilityStatus = media::mojom::AvailabilityStatus;
using Quality = media::mojom::SpeechRecognitionQuality;

}  // namespace

class BraveOnDeviceSpeechAvailabilityUnitTest : public testing::Test {
 protected:
  // Initialized before the task environment starts its threads.
  base::test::ScopedFeatureList feature_list_{
      local_ai::kBraveOnDeviceSpeechRecognition};
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, DownloadableUntilInstalled) {
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "en-US",
                                               Quality::kCommand));

  state->SetInstallDir(
      base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  EXPECT_EQ(AvailabilityStatus::kAvailable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "en-US",
                                               Quality::kCommand));

  state->SetInstallDir(base::FilePath());
}

// Private, Tor and Guest profiles get none, also with the model on disk.
TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest,
       UnavailableOutsideRegularProfiles) {
  auto* state = local_ai::OnDeviceSpeechModelsState::GetInstance();
  state->SetInstallDir(
      base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  ASSERT_EQ(AvailabilityStatus::kAvailable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "en-US",
                                               Quality::kCommand));

  TestingProfile::Builder guest_builder;
  guest_builder.SetGuestSession();
  std::unique_ptr<TestingProfile> guest = guest_builder.Build();
  const std::pair<std::string_view, Profile*> profiles[] = {
      {"Private", profile_.GetPrimaryOTRProfile(/*create_if_needed=*/true)},
      {"Tor", profile_.GetOffTheRecordProfile(Profile::OTRProfileID::TorID(),
                                              /*create_if_needed=*/true)},
      {"Guest", guest->GetPrimaryOTRProfile(/*create_if_needed=*/true)}};
  for (const auto& [name, profile] : profiles) {
    SCOPED_TRACE(name);
    EXPECT_EQ(AvailabilityStatus::kUnavailable,
              GetBraveOnDeviceSpeechAvailability(profile, "en-US",
                                                 Quality::kCommand));
  }

  state->SetInstallDir(base::FilePath());
}

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, UnavailableWhenLocalAIOff) {
  PrefService* local_state = TestingBrowserProcess::GetGlobal()->local_state();
  local_state->SetBoolean(local_ai::prefs::kBraveLocalAIEnabled, false);
  EXPECT_EQ(AvailabilityStatus::kUnavailable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "en-US",
                                               Quality::kCommand));
  local_state->ClearPref(local_ai::prefs::kBraveLocalAIEnabled);
}

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, UnavailableWhenFeatureOff) {
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(local_ai::kBraveOnDeviceSpeechRecognition);
  EXPECT_EQ(AvailabilityStatus::kUnavailable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "en-US",
                                               Quality::kCommand));
}

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, ServesCommandAndDictation) {
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "en-US",
                                               Quality::kCommand));
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "en-US",
                                               Quality::kDictation));
  EXPECT_EQ(AvailabilityStatus::kUnavailable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "en-US",
                                               Quality::kConversation));
}

TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, EnglishLocalesOnly) {
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "en-US",
                                               Quality::kCommand));
  EXPECT_EQ(AvailabilityStatus::kDownloadable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "EN-GB",
                                               Quality::kCommand));
  EXPECT_EQ(
      AvailabilityStatus::kDownloadable,
      GetBraveOnDeviceSpeechAvailability(&profile_, "en", Quality::kCommand));
  EXPECT_EQ(AvailabilityStatus::kUnavailable,
            GetBraveOnDeviceSpeechAvailability(&profile_, "fr-FR",
                                               Quality::kCommand));
  EXPECT_EQ(
      AvailabilityStatus::kUnavailable,
      GetBraveOnDeviceSpeechAvailability(&profile_, "", Quality::kCommand));
}

}  // namespace speech
