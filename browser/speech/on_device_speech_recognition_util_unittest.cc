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
#include "components/prefs/testing_pref_service.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speech {

// Declared rather than included, the way the chromium_src override of
// chrome/browser/speech/on_device_speech_recognition_util.cc declares it. The
// policy has no header, so the linker is what checks this against the
// definition in //brave/browser/speech:chromium_impl.
media::mojom::AvailabilityStatus GetBraveOnDeviceSpeechAvailability(
    std::string_view language,
    media::mojom::SpeechRecognitionQuality quality);

namespace {

using AvailabilityStatus = media::mojom::AvailabilityStatus;
using Quality = media::mojom::SpeechRecognitionQuality;

}  // namespace

// Both SpeechRecognition.available() and SpeechRecognition.start() resolve
// here, and they do not agree on everything they pass, which is what the
// language and quality cases below are about.
class BraveOnDeviceSpeechAvailabilityUnitTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        local_ai::kBraveOnDeviceSpeechRecognition);
  }

  void TearDown() override {
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath());
    local_state()->SetBoolean(local_ai::prefs::kBraveLocalAIEnabled, true);
  }

 protected:
  PrefService* local_state() {
    return TestingBrowserProcess::GetGlobal()->GetTestingLocalState();
  }

  void InstallModel() {
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  }

  AvailabilityStatus Availability(std::string_view language) {
    return GetBraveOnDeviceSpeechAvailability(language, Quality::kCommand);
  }

  base::test::ScopedFeatureList feature_list_;
};

// Tests that the model being on disk is the difference between offering the
// install and answering yes. Reporting kDownloadable is what lets a page drive
// the install flow.
TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, DownloadableUntilInstalled) {
  EXPECT_EQ(AvailabilityStatus::kDownloadable, Availability("en-US"));

  InstallModel();
  EXPECT_EQ(AvailabilityStatus::kAvailable, Availability("en-US"));
}

// Tests that the local AI umbrella switches this off with everything else.
TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, UnavailableWhenLocalAIOff) {
  InstallModel();
  local_state()->SetBoolean(local_ai::prefs::kBraveLocalAIEnabled, false);

  EXPECT_EQ(AvailabilityStatus::kUnavailable, Availability("en-US"));
}

// Tests that with the feature off there is nothing to report, because Brave
// ships no upstream on-device backend to fall back to.
TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, UnavailableWhenFeatureOff) {
  InstallModel();
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(local_ai::kBraveOnDeviceSpeechRecognition);

  EXPECT_EQ(AvailabilityStatus::kUnavailable, Availability("en-US"));
}

// Tests the qualities Brave's own model serves. kDictation arrives raw from
// SpeechRecognition.start(), which is the only entry point that does not go
// through the plaster normalising it to kCommand, so this case is the one
// keeping start({quality:'dictation'}) working.
TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, ServesCommandAndDictation) {
  InstallModel();

  EXPECT_EQ(AvailabilityStatus::kAvailable,
            GetBraveOnDeviceSpeechAvailability("en-US", Quality::kCommand));
  EXPECT_EQ(AvailabilityStatus::kAvailable,
            GetBraveOnDeviceSpeechAvailability("en-US", Quality::kDictation));
  EXPECT_EQ(
      AvailabilityStatus::kUnavailable,
      GetBraveOnDeviceSpeechAvailability("en-US", Quality::kConversation));
}

// Tests that the model's one language is answered for, and nothing else is.
TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, EnglishOnly) {
  InstallModel();

  EXPECT_EQ(AvailabilityStatus::kAvailable, Availability("en-US"));
  EXPECT_EQ(AvailabilityStatus::kUnavailable, Availability("fr-FR"));
  EXPECT_EQ(AvailabilityStatus::kUnavailable, Availability(""));
}

// Tests that the language compare does not depend on case. available()
// canonicalises before asking, but start() passes the page's lang through
// untouched, so a case-sensitive compare made the two disagree.
TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, LanguageCompareIgnoresCase) {
  InstallModel();

  EXPECT_EQ(AvailabilityStatus::kAvailable, Availability("EN-US"));
  EXPECT_EQ(AvailabilityStatus::kAvailable, Availability("En-us"));
}

// Tests that every English locale is answered for, not just the one the model
// is trained on. Upstream's SODA path compares primary subtags in one place
// and exact locales in another, and this is deliberately the broader of the
// two so that the two entry points cannot disagree.
TEST_F(BraveOnDeviceSpeechAvailabilityUnitTest, AllEnglishLocales) {
  EXPECT_EQ(AvailabilityStatus::kDownloadable, Availability("en-GB"));

  InstallModel();
  EXPECT_EQ(AvailabilityStatus::kAvailable, Availability("en-GB"));
  EXPECT_EQ(AvailabilityStatus::kAvailable, Availability("en"));
}

}  // namespace speech
