/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/speech/brave_soda_installer.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/test/mock_permission_prompt_factory.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/web_contents_tester.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace speech {

namespace {

using ResponseType = permissions::PermissionRequestManager::AutoResponseType;

}  // namespace

// What `install()` prompts for before Brave's model is downloaded. No
// component registrar runs here, so a download that starts answers false.
class BraveOnDeviceSpeechInstallPromptUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  BraveOnDeviceSpeechInstallPromptUnitTest() {
    feature_list_.InitAndEnableFeature(
        local_ai::kBraveOnDeviceSpeechRecognition);
  }

 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    soda_installer_ = std::make_unique<BraveSodaInstaller>();
  }

  void TearDown() override {
    prompt_factory_.reset();
    DeleteContents();
    soda_installer_.reset();
    local_state()->ClearPref(local_ai::prefs::kOnDeviceSpeechModelEnabled);
    ChromeRenderViewHostTestHarness::TearDown();
  }

  PrefService* local_state() {
    return TestingBrowserProcess::GetGlobal()->local_state();
  }

  // Calls `install()` from a page that answers the prompt with `response`.
  void Install(const std::vector<std::string>& languages,
               ResponseType response = ResponseType::ACCEPT_ALL) {
    NavigateAndCommit(GURL("https://example.com/"));
    permissions::PermissionRequestManager::CreateForWebContents(web_contents());
    prompt_factory_ =
        std::make_unique<permissions::MockPermissionPromptFactory>(
            permissions::PermissionRequestManager::FromWebContents(
                web_contents()));
    prompt_factory_->set_response_type(response);

    base::test::TestFuture<bool> installed;
    OnDeviceSpeechRecognitionImpl::GetOrCreateForCurrentDocument(main_rfh())
        ->Install(languages, media::mojom::SpeechRecognitionQuality::kCommand,
                  installed.GetCallback());
    ASSERT_TRUE(installed.Wait());
  }

  int show_count() { return prompt_factory_->show_count(); }

  bool IsEnabled() {
    return local_state()->GetBoolean(
        local_ai::prefs::kOnDeviceSpeechModelEnabled);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  std::unique_ptr<BraveSodaInstaller> soda_installer_;
  std::unique_ptr<permissions::MockPermissionPromptFactory> prompt_factory_;
};

// `install()` runs again after Allow, and does not prompt again.
TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, AllowEnablesTheModel) {
  Install({"en-US"});
  EXPECT_EQ(1, show_count());
  EXPECT_TRUE(IsEnabled());
}

TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, DenyPromptsOnce) {
  Install({"en-US"}, ResponseType::DENY_ALL);
  EXPECT_EQ(1, show_count());
}

TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, NoPromptWhenEnabled) {
  local_state()->SetBoolean(local_ai::prefs::kOnDeviceSpeechModelEnabled, true);
  Install({"en-US"});
  EXPECT_EQ(0, show_count());
}

// Upstream refuses these, so a prompt would be for nothing.
TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, NoPromptForWhatIsRefused) {
  Install({});
  EXPECT_EQ(0, show_count());
  Install({"en-US", "fr-FR"});
  EXPECT_EQ(0, show_count());
}

TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, NoPromptAfterDontAskAgain) {
  profile()->GetPrefs()->SetBoolean(
      local_ai::prefs::kAskEnableOnDeviceSpeechModel, false);
  Install({"en-US"});
  EXPECT_EQ(0, show_count());
}

TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, NoPromptInTor) {
  SetContents(content::WebContentsTester::CreateTestWebContents(
      profile()->GetOffTheRecordProfile(Profile::OTRProfileID::TorID(),
                                        /*create_if_needed=*/true),
      nullptr));
  Install({"en-US"});
  EXPECT_EQ(0, show_count());
}

}  // namespace speech
