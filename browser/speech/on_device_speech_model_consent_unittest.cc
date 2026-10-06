/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_model_consent.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/test/mock_permission_prompt_factory.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace speech {

namespace {

using Quality = media::mojom::SpeechRecognitionQuality;
using ResponseType = permissions::PermissionRequestManager::AutoResponseType;

std::vector<std::string> English() {
  return {"en-US"};
}

// Stands in for `Install` running again once the user agrees, so only an
// agreement answers true.
void InstallSucceeds(base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(true);
}

}  // namespace

class OnDeviceSpeechModelConsentUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  OnDeviceSpeechModelConsentUnitTest() {
    feature_list_.InitAndEnableFeature(
        local_ai::kBraveOnDeviceSpeechRecognition);
  }

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    NavigateAndCommit(GURL("https://example.com/"));
    permissions::PermissionRequestManager::CreateForWebContents(web_contents());
    prompt_factory_ =
        std::make_unique<permissions::MockPermissionPromptFactory>(
            permissions::PermissionRequestManager::FromWebContents(
                web_contents()));
  }

  void TearDown() override {
    prompt_factory_.reset();
    // Process-wide, so leave them as a fresh browser would have them.
    local_state()->ClearPref(local_ai::prefs::kBraveLocalAIEnabled);
    local_state()->ClearPref(local_ai::prefs::kOnDeviceSpeechModelEnabled);
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath());
    ChromeRenderViewHostTestHarness::TearDown();
  }

 protected:
  PrefService* local_state() {
    return TestingBrowserProcess::GetGlobal()->local_state();
  }

  bool IsEnabled() {
    return local_state()->GetBoolean(
        local_ai::prefs::kOnDeviceSpeechModelEnabled);
  }

  // Asks as `Install` does, and waits for the answer.
  bool Request() {
    base::test::TestFuture<bool> answer;
    RequestBraveOnDeviceSpeechModelConsent(
        *main_rfh(), base::BindOnce(&InstallSucceeds), answer.GetCallback());
    return answer.Get();
  }

  std::unique_ptr<permissions::MockPermissionPromptFactory> prompt_factory_;

 private:
  base::test::ScopedFeatureList feature_list_;
};

// `ShouldRequestBraveOnDeviceSpeechModelConsent`, which `Install` asks before
// it downloads anything. It says yes for exactly the requests that would start
// a download.

TEST_F(OnDeviceSpeechModelConsentUnitTest, ShouldRequestForADownload) {
  EXPECT_TRUE(ShouldRequestBraveOnDeviceSpeechModelConsent(English(),
                                                           Quality::kCommand));
  // `Install` rewrites dictation to command, but asks either way.
  EXPECT_TRUE(ShouldRequestBraveOnDeviceSpeechModelConsent({"en-US", "en-GB"},
                                                           Quality::kCommand));
}

// Upstream refuses these, so asking would show a prompt for nothing.
TEST_F(OnDeviceSpeechModelConsentUnitTest,
       ShouldNotRequestWhatUpstreamRefuses) {
  EXPECT_FALSE(
      ShouldRequestBraveOnDeviceSpeechModelConsent({}, Quality::kCommand));
  EXPECT_FALSE(ShouldRequestBraveOnDeviceSpeechModelConsent({"fr-FR"},
                                                            Quality::kCommand));
  // One language Brave does not serve is enough to refuse the whole request.
  EXPECT_FALSE(ShouldRequestBraveOnDeviceSpeechModelConsent({"en-US", "fr-FR"},
                                                            Quality::kCommand));
  EXPECT_FALSE(ShouldRequestBraveOnDeviceSpeechModelConsent(
      English(), Quality::kConversation));
}

TEST_F(OnDeviceSpeechModelConsentUnitTest, ShouldNotRequestWhenNotAllowed) {
  local_state()->SetBoolean(local_ai::prefs::kBraveLocalAIEnabled, false);
  EXPECT_FALSE(ShouldRequestBraveOnDeviceSpeechModelConsent(English(),
                                                            Quality::kCommand));
}

TEST_F(OnDeviceSpeechModelConsentUnitTest, ShouldNotRequestWhenFeatureOff) {
  base::test::ScopedFeatureList disabled;
  disabled.InitAndDisableFeature(local_ai::kBraveOnDeviceSpeechRecognition);
  EXPECT_FALSE(ShouldRequestBraveOnDeviceSpeechModelConsent(English(),
                                                            Quality::kCommand));
}

// The model is already enabled. This is what lets `Install` run again, once the
// prompt is allowed, and what asks for a download that failed again without
// asking the user.
TEST_F(OnDeviceSpeechModelConsentUnitTest, ShouldNotRequestOnceEnabled) {
  local_state()->SetBoolean(local_ai::prefs::kOnDeviceSpeechModelEnabled, true);
  EXPECT_FALSE(ShouldRequestBraveOnDeviceSpeechModelConsent(English(),
                                                            Quality::kCommand));
}

TEST_F(OnDeviceSpeechModelConsentUnitTest, ShouldNotRequestWithAModel) {
  local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
      base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  EXPECT_FALSE(ShouldRequestBraveOnDeviceSpeechModelConsent(English(),
                                                            Quality::kCommand));
}

// `RequestBraveOnDeviceSpeechModelConsent`, which asks the user.

TEST_F(OnDeviceSpeechModelConsentUnitTest, AllowEnablesTheModel) {
  prompt_factory_->set_response_type(ResponseType::ACCEPT_ALL);

  EXPECT_TRUE(Request());
  EXPECT_EQ(1, prompt_factory_->show_count());
  EXPECT_TRUE(IsEnabled());
}

TEST_F(OnDeviceSpeechModelConsentUnitTest, DenyDoesNotEnable) {
  prompt_factory_->set_response_type(ResponseType::DENY_ALL);

  EXPECT_FALSE(Request());
  EXPECT_EQ(1, prompt_factory_->show_count());
  EXPECT_FALSE(IsEnabled());
}

// Blocking saves nothing unless "Don't ask again" is checked, so a site that
// asks again is answered again.
TEST_F(OnDeviceSpeechModelConsentUnitTest, DenyWithoutDontAskAgainAsksAgain) {
  prompt_factory_->set_response_type(ResponseType::DENY_ALL);

  EXPECT_FALSE(Request());
  EXPECT_FALSE(Request());
  EXPECT_EQ(2, prompt_factory_->show_count());
  EXPECT_FALSE(IsEnabled());
}

// Dismissing saves nothing, so a site that asks again is answered again.
TEST_F(OnDeviceSpeechModelConsentUnitTest, DismissAsksAgain) {
  prompt_factory_->set_response_type(ResponseType::DISMISS);

  EXPECT_FALSE(Request());
  EXPECT_FALSE(Request());
  EXPECT_EQ(2, prompt_factory_->show_count());
  EXPECT_FALSE(IsEnabled());
}

TEST_F(OnDeviceSpeechModelConsentUnitTest, DontAskAgainRefusesWithoutAsking) {
  profile()->GetPrefs()->SetBoolean(
      local_ai::prefs::kAskEnableOnDeviceSpeechModel, false);
  prompt_factory_->set_response_type(ResponseType::ACCEPT_ALL);

  EXPECT_FALSE(Request());
  EXPECT_EQ(0, prompt_factory_->show_count());
  EXPECT_FALSE(IsEnabled());
}

// The choice belongs to one profile.
TEST_F(OnDeviceSpeechModelConsentUnitTest, DontAskAgainIsPerProfile) {
  std::unique_ptr<TestingProfile> other = TestingProfile::Builder().Build();
  other->GetPrefs()->SetBoolean(local_ai::prefs::kAskEnableOnDeviceSpeechModel,
                                false);
  prompt_factory_->set_response_type(ResponseType::ACCEPT_ALL);

  EXPECT_TRUE(Request());
  EXPECT_EQ(1, prompt_factory_->show_count());
}

}  // namespace speech
