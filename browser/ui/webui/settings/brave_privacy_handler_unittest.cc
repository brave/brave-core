// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"

#include <memory>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kGetMessage[] = "getOnDeviceSpeechModelEnabled";
constexpr char kSetMessage[] = "setOnDeviceSpeechModelEnabled";
constexpr char kChangedEvent[] = "on-device-speech-model-enabled-changed";

}  // namespace

class BravePrivacyHandlerOnDeviceSpeechUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  BravePrivacyHandlerOnDeviceSpeechUnitTest() {
    feature_list_.InitAndEnableFeature(
        local_ai::kBraveOnDeviceSpeechRecognition);
  }

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    test_web_ui_ = std::make_unique<content::TestWebUI>();
    test_web_ui_->set_web_contents(web_contents());
    test_web_ui_->AddMessageHandler(std::make_unique<BravePrivacyHandler>());
  }

  void TearDown() override {
    test_web_ui_.reset();
    // Process-wide, so leave them as a fresh browser would have them.
    local_state()->ClearPref(local_ai::prefs::kBraveLocalAIEnabled);
    local_state()->ClearPref(local_ai::prefs::kOnDeviceSpeechModelEnabled);
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

  void Set(base::Value value) {
    test_web_ui_->HandleReceivedMessage(
        kSetMessage, base::ListValue().Append(std::move(value)));
  }

  base::test::ScopedFeatureList feature_list_;
  std::unique_ptr<content::TestWebUI> test_web_ui_;
};

TEST_F(BravePrivacyHandlerOnDeviceSpeechUnitTest, SetEnablesAndDisables) {
  EXPECT_FALSE(IsEnabled());

  Set(base::Value(true));
  EXPECT_TRUE(IsEnabled());

  Set(base::Value(false));
  EXPECT_FALSE(IsEnabled());
}

TEST_F(BravePrivacyHandlerOnDeviceSpeechUnitTest, SetIgnoresANonBoolean) {
  Set(base::Value("true"));
  EXPECT_FALSE(IsEnabled());
}

// The registrar leaves the pref alone while the switch is off, so a true
// written then would stay and download once the switch came back on.
TEST_F(BravePrivacyHandlerOnDeviceSpeechUnitTest, SetIgnoredWhileLocalAIIsOff) {
  local_state()->SetBoolean(local_ai::prefs::kBraveLocalAIEnabled, false);

  Set(base::Value(true));
  EXPECT_FALSE(IsEnabled());
}

TEST_F(BravePrivacyHandlerOnDeviceSpeechUnitTest, SetIgnoredWhileFeatureIsOff) {
  feature_list_.Reset();
  feature_list_.InitAndDisableFeature(
      local_ai::kBraveOnDeviceSpeechRecognition);

  Set(base::Value(true));
  EXPECT_FALSE(IsEnabled());
}

TEST_F(BravePrivacyHandlerOnDeviceSpeechUnitTest, GetResolvesWithThePref) {
  local_state()->SetBoolean(local_ai::prefs::kOnDeviceSpeechModelEnabled, true);

  test_web_ui_->HandleReceivedMessage(kGetMessage,
                                      base::ListValue().Append("callback-id"));

  const content::TestWebUI::CallData& call_data =
      *test_web_ui_->call_data().back();
  EXPECT_EQ("cr.webUIResponse", call_data.function_name());
  EXPECT_EQ("callback-id", call_data.arg1()->GetString());
  EXPECT_TRUE(call_data.arg2()->GetBool());
  EXPECT_TRUE(call_data.arg3()->GetBool());
}

// Turning Local AI off clears the pref, which the open page has to show.
TEST_F(BravePrivacyHandlerOnDeviceSpeechUnitTest, ChangeIsPushedToThePage) {
  // Asking for the value is what lets the handler talk to the page.
  test_web_ui_->HandleReceivedMessage(kGetMessage,
                                      base::ListValue().Append("callback-id"));

  local_state()->SetBoolean(local_ai::prefs::kOnDeviceSpeechModelEnabled, true);

  const content::TestWebUI::CallData& call_data =
      *test_web_ui_->call_data().back();
  EXPECT_EQ("cr.webUIListenerCallback", call_data.function_name());
  EXPECT_EQ(kChangedEvent, call_data.arg1()->GetString());
  EXPECT_TRUE(call_data.arg2()->GetBool());
}
