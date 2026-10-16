/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_model_permission_request.h"

#include <memory>
#include <variant>

#include "base/test/test_future.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace speech {

class OnDeviceSpeechModelPermissionRequestUnitTest : public testing::Test {
 public:
  void SetUp() override {
    local_ai::prefs::RegisterLocalStatePrefs(local_state_.registry());
    local_ai::prefs::RegisterProfilePrefs(profile_prefs_.registry());
  }

 protected:
  std::unique_ptr<OnDeviceSpeechModelPermissionRequest> CreateRequest() {
    return std::make_unique<OnDeviceSpeechModelPermissionRequest>(
        local_state_, profile_prefs_, GURL("https://example.com"),
        allowed_.GetCallback());
  }

  bool IsEnabled() {
    return local_state_.GetBoolean(
        local_ai::prefs::kOnDeviceSpeechModelEnabled);
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
  EXPECT_TRUE(IsEnabled());
}

// "Don't ask again" is kept only on a block.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, DenyWithDontAskAgain) {
  auto request = CreateRequest();
  request->set_dont_ask_again(true);
  request->PermissionDenied();

  ASSERT_TRUE(allowed_.IsReady());
  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled());
  EXPECT_FALSE(IsAskEnabled());
}

// A plain block must not stop the prompt for good.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest,
       DenyWithoutDontAskAgainKeepsAsking) {
  auto request = CreateRequest();
  request->PermissionDenied();

  ASSERT_TRUE(allowed_.IsReady());
  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled());
  EXPECT_TRUE(IsAskEnabled());
}

// Dismissing saves nothing, even with the checkbox checked.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, DismissSavesNothing) {
  auto request = CreateRequest();
  request->set_dont_ask_again(true);
  request->Cancelled(/*is_final_decision=*/true);

  ASSERT_TRUE(allowed_.IsReady());
  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled());
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
