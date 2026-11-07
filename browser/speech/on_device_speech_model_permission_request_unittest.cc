/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_model_permission_request.h"

#include <memory>
#include <variant>

#include "base/test/test_future.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "brave/components/local_ai/core/utils.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace speech {

namespace {

using local_ai::OnDeviceSpeechModel;

}  // namespace

class OnDeviceSpeechModelPermissionRequestUnitTest : public testing::Test {
 public:
  void SetUp() override {
    local_ai::prefs::RegisterLocalStatePrefs(local_state_.registry());
    local_ai::prefs::RegisterProfilePrefs(profile_prefs_.registry());
  }

 protected:
  std::unique_ptr<permissions::PermissionRequest> CreateRequest(
      OnDeviceSpeechModel model = OnDeviceSpeechModel::kEnglish,
      const GURL& origin = GURL("https://example.com")) {
    return std::make_unique<OnDeviceSpeechModelPermissionRequest>(
        model, local_state_, profile_prefs_, origin, allowed_.GetCallback());
  }

  bool IsEnabled(OnDeviceSpeechModel model) {
    return local_state_.GetBoolean(
        local_ai::GetOnDeviceSpeechModelEnabledPref(model));
  }

  bool IsAskEnabled() {
    return profile_prefs_.GetBoolean(
        local_ai::prefs::kAskEnableOnDeviceSpeechModel);
  }

  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  base::test::TestFuture<bool> allowed_;
};

TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, AllowEnablesTheModel) {
  auto request = CreateRequest();
  request->PermissionGranted(std::monostate(), /*is_one_time=*/false);

  ASSERT_TRUE(allowed_.IsReady());
  EXPECT_TRUE(allowed_.Get());
  EXPECT_TRUE(IsEnabled(OnDeviceSpeechModel::kEnglish));
}

TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest,
       AllowEnablesOnlyTheModelAskedFor) {
  auto request = CreateRequest(OnDeviceSpeechModel::kMultilingual);
  request->PermissionGranted(std::monostate(), /*is_one_time=*/false);

  ASSERT_TRUE(allowed_.IsReady());
  EXPECT_TRUE(allowed_.Get());
  EXPECT_TRUE(IsEnabled(OnDeviceSpeechModel::kMultilingual));
  EXPECT_FALSE(IsEnabled(OnDeviceSpeechModel::kEnglish));
}

// The prompt names the model it asks for, with that model's size.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest,
       MessageNamesTheModelAndItsSize) {
  EXPECT_EQ(
      u"Download a speech recognition model (636 MB) to process speech on "
      u"this device",
      CreateRequest()->GetMessageTextFragment());
  EXPECT_EQ(
      u"Download a multilingual speech recognition model (697 MB) to process "
      u"speech on this device",
      CreateRequest(OnDeviceSpeechModel::kMultilingual)
          ->GetMessageTextFragment());
}

// Only a request for the same model from the same origin is a duplicate. A
// duplicate joins the open prompt and gets its answer.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest,
       DuplicateOnlyForTheSameModelAndOrigin) {
  auto english = CreateRequest();

  EXPECT_TRUE(english->IsDuplicateOf(CreateRequest().get()));
  EXPECT_FALSE(english->IsDuplicateOf(
      CreateRequest(OnDeviceSpeechModel::kMultilingual).get()));
  EXPECT_FALSE(english->IsDuplicateOf(
      CreateRequest(OnDeviceSpeechModel::kEnglish, GURL("https://other.com"))
          .get()));
}

// "Don't ask again" is kept only on a block.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, DenyWithDontAskAgain) {
  auto request = CreateRequest();
  request->set_dont_ask_again(true);
  request->PermissionDenied();

  ASSERT_TRUE(allowed_.IsReady());
  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled(OnDeviceSpeechModel::kEnglish));
  EXPECT_FALSE(IsAskEnabled());
}

// A plain block must not stop the prompt for good.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest,
       DenyWithoutDontAskAgainKeepsAsking) {
  auto request = CreateRequest();
  request->PermissionDenied();

  ASSERT_TRUE(allowed_.IsReady());
  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled(OnDeviceSpeechModel::kEnglish));
  EXPECT_TRUE(IsAskEnabled());
}

// Dismissing saves nothing, even with the checkbox checked.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, DismissSavesNothing) {
  auto request = CreateRequest();
  request->set_dont_ask_again(true);
  request->Cancelled(/*is_final_decision=*/true);

  ASSERT_TRUE(allowed_.IsReady());
  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled(OnDeviceSpeechModel::kEnglish));
  EXPECT_TRUE(IsAskEnabled());
}

// A non-final cancel is not a decision. A final decision can follow.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, NonFinalCancelWaits) {
  auto request = CreateRequest();
  request->Cancelled(/*is_final_decision=*/false);
  EXPECT_FALSE(allowed_.IsReady());

  request->PermissionGranted(std::monostate(), /*is_one_time=*/false);
  ASSERT_TRUE(allowed_.IsReady());
  EXPECT_TRUE(allowed_.Get());
}

}  // namespace speech
