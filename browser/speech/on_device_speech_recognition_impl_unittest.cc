/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/on_device_speech_recognition_impl.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/speech/brave_soda_installer.h"
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
    guest_profile_.reset();
    soda_installer_.reset();
    local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
        base::FilePath());
    local_state()->ClearPref(local_ai::prefs::kOnDeviceSpeechModelEnabled);
    ChromeRenderViewHostTestHarness::TearDown();
  }

  PrefService* local_state() {
    return TestingBrowserProcess::GetGlobal()->local_state();
  }

  // A Private, a Tor and a Guest profile, each with its name.
  std::vector<std::pair<std::string_view, Profile*>> NonRegularProfiles() {
    if (!guest_profile_) {
      TestingProfile::Builder builder;
      builder.SetGuestSession();
      guest_profile_ = builder.Build();
    }
    return {
        {"Private", profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true)},
        {"Tor", profile()->GetOffTheRecordProfile(
                    Profile::OTRProfileID::TorID(), /*create_if_needed=*/true)},
        {"Guest",
         guest_profile_->GetPrimaryOTRProfile(/*create_if_needed=*/true)}};
  }

  // Opens the next page in a new tab of `profile`.
  void UseProfile(Profile* profile) {
    prompt_factory_.reset();
    SetContents(
        content::WebContentsTester::CreateTestWebContents(profile, nullptr));
  }

  // Opens a page that answers the prompt with `response`.
  void OpenPage(ResponseType response) {
    NavigateAndCommit(GURL("https://example.com/"));
    permissions::PermissionRequestManager::CreateForWebContents(web_contents());
    prompt_factory_ =
        std::make_unique<permissions::MockPermissionPromptFactory>(
            permissions::PermissionRequestManager::FromWebContents(
                web_contents()));
    prompt_factory_->set_response_type(response);
  }

  // Calls `install()` from the open page, and does not wait for the answer.
  void StartInstall(const std::vector<std::string>& languages,
                    OnDeviceSpeechRecognitionImpl::InstallCallback callback) {
    OnDeviceSpeechRecognitionImpl::GetOrCreateForCurrentDocument(main_rfh())
        ->Install(languages, media::mojom::SpeechRecognitionQuality::kCommand,
                  std::move(callback));
  }

  // Calls `install()` from a page that answers the prompt with `response`, and
  // returns the answer.
  bool Install(const std::vector<std::string>& languages,
               ResponseType response = ResponseType::ACCEPT_ALL) {
    OpenPage(response);
    base::test::TestFuture<bool> installed;
    StartInstall(languages, installed.GetCallback());
    return installed.Get();
  }

  int show_count() { return prompt_factory_->show_count(); }

  bool IsEnabled() {
    return local_state()->GetBoolean(
        local_ai::prefs::kOnDeviceSpeechModelEnabled);
  }

  bool AsksAgain() {
    return profile()->GetPrefs()->GetBoolean(
        local_ai::prefs::kAskEnableOnDeviceSpeechModel);
  }

  std::unique_ptr<permissions::MockPermissionPromptFactory> prompt_factory_;

 private:
  base::test::ScopedFeatureList feature_list_;
  std::unique_ptr<BraveSodaInstaller> soda_installer_;
  std::unique_ptr<TestingProfile> guest_profile_;
};

// `install()` runs again after Allow, and does not prompt again.
TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, AllowEnablesTheModel) {
  Install({"en-US"});
  EXPECT_EQ(1, show_count());
  EXPECT_TRUE(IsEnabled());
}

TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, DenyPromptsOnce) {
  EXPECT_FALSE(Install({"en-US"}, ResponseType::DENY_ALL));
  EXPECT_EQ(1, show_count());
  EXPECT_FALSE(IsEnabled());
}

TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, NoPromptWhenEnabled) {
  local_state()->SetBoolean(local_ai::prefs::kOnDeviceSpeechModelEnabled, true);
  Install({"en-US"});
  EXPECT_EQ(0, show_count());
}

// Upstream refuses these, so a prompt would be for nothing.
TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, NoPromptForWhatIsRefused) {
  EXPECT_FALSE(Install({}));
  EXPECT_EQ(0, show_count());
  EXPECT_FALSE(Install({"en-US", "fr-FR"}));
  EXPECT_EQ(0, show_count());
}

TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, NoPromptAfterDontAskAgain) {
  profile()->GetPrefs()->SetBoolean(
      local_ai::prefs::kAskEnableOnDeviceSpeechModel, false);
  EXPECT_FALSE(Install({"en-US"}));
  EXPECT_EQ(0, show_count());
}

// Private, Tor and Guest profiles get no model, so `install()` gets false
// without a prompt.
TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest,
       NoPromptOutsideRegularProfiles) {
  for (const auto& [name, profile] : NonRegularProfiles()) {
    SCOPED_TRACE(name);
    UseProfile(profile);
    EXPECT_FALSE(Install({"en-US"}));
    EXPECT_EQ(0, show_count());
  }
  EXPECT_FALSE(IsEnabled());
}

// They get false with the model turned on and on disk too, where a regular
// profile gets true.
TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest,
       RefusedOutsideRegularProfilesWithTheModel) {
  local_state()->SetBoolean(local_ai::prefs::kOnDeviceSpeechModelEnabled, true);
  local_ai::OnDeviceSpeechModelsState::GetInstance()->SetInstallDir(
      base::FilePath(FILE_PATH_LITERAL("/brave/speech/models")));
  EXPECT_TRUE(Install({"en-US"}));

  for (const auto& [name, profile] : NonRegularProfiles()) {
    SCOPED_TRACE(name);
    UseProfile(profile);
    EXPECT_FALSE(Install({"en-US"}));
    EXPECT_EQ(0, show_count());
  }
}

// A tab closing with the prompt open still answers `install()`, and saves
// nothing.
TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, TabClosedWithPromptOpen) {
  OpenPage(ResponseType::NONE);
  base::test::TestFuture<bool> installed;
  StartInstall({"en-US"}, installed.GetCallback());
  prompt_factory_->WaitForPermissionBubble();
  ASSERT_FALSE(installed.IsReady());

  DeleteContents();

  ASSERT_TRUE(installed.IsReady());
  EXPECT_FALSE(installed.Get());
  EXPECT_FALSE(IsEnabled());
  EXPECT_TRUE(AsksAgain());
}

// A second `install()` while the prompt is open joins it, and both run again
// once the user allows.
TEST_F(BraveOnDeviceSpeechInstallPromptUnitTest, TwoInstallsShareOnePrompt) {
  OpenPage(ResponseType::ACCEPT_ALL);
  base::test::TestFuture<bool> first;
  base::test::TestFuture<bool> second;
  StartInstall({"en-US"}, first.GetCallback());
  StartInstall({"en-US"}, second.GetCallback());

  ASSERT_TRUE(first.Wait());
  ASSERT_TRUE(second.Wait());
  EXPECT_EQ(1, show_count());
  EXPECT_TRUE(IsEnabled());
}

}  // namespace speech
