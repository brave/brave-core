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
#include "brave/grit/brave_generated_resources.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/text/bytes_formatting.h"
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
      OnDeviceSpeechModel model = OnDeviceSpeechModel::kEnglish) {
    return std::make_unique<OnDeviceSpeechModelPermissionRequest>(
        model, local_state_, profile_prefs_, GURL("https://example.com"),
        allowed_.GetCallback());
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
  EXPECT_FALSE(IsEnabled(OnDeviceSpeechModel::kMultilingual));
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
      l10n_util::GetStringFUTF16(IDS_ON_DEVICE_SPEECH_MODEL_PERMISSION_FRAGMENT,
                                 ui::FormatBytes(local_ai::kApproxModelSize)),
      CreateRequest()->GetMessageTextFragment());
  EXPECT_EQ(l10n_util::GetStringFUTF16(
                IDS_ON_DEVICE_SPEECH_MULTILINGUAL_MODEL_PERMISSION_FRAGMENT,
                ui::FormatBytes(local_ai::kApproxMultilingualModelSize)),
            CreateRequest(OnDeviceSpeechModel::kMultilingual)
                ->GetMessageTextFragment());
}

// A request for the other model must not join an open prompt and get its
// answer.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest,
       DuplicateOnlyForTheSameModel) {
  auto english = CreateRequest();
  auto multilingual = CreateRequest(OnDeviceSpeechModel::kMultilingual);

  EXPECT_TRUE(english->IsDuplicateOf(CreateRequest().get()));
  EXPECT_FALSE(english->IsDuplicateOf(multilingual.get()));
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
