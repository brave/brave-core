/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_model_permission_request.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "base/test/test_future.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/permissions/request_type.h"
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

TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, IsOfItsOwnType) {
  auto request = CreateRequest();
  EXPECT_EQ(permissions::RequestType::kBraveOnDeviceSpeechModel,
            request->request_type());
  // No content setting to key an embargo on.
  EXPECT_FALSE(request->uses_automatic_embargo());
}

// The message names the size of the download, from the formatted constant.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, MessageNamesTheSize) {
  auto request = CreateRequest();
  // Private on the request, and called by the permission bubble through the
  // base class.
  const permissions::PermissionRequest& base_request = *request;
  const std::u16string message = base_request.GetMessageTextFragment();
  EXPECT_NE(std::u16string::npos, message.find(u"MB")) << message;
}

// Allow is the only thing that makes the registrar register the model.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, AllowRequestsTheModel) {
  auto request = CreateRequest();
  request->PermissionGranted(std::monostate(), /*is_one_time=*/false);

  EXPECT_TRUE(allowed_.Get());
  EXPECT_TRUE(IsEnabled());
  EXPECT_TRUE(IsAskEnabled());
}

// The model is shared and kept, so allowing it once is allowing it.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest,
       AllowThisTimeRequestsTheModel) {
  auto request = CreateRequest();
  request->PermissionGranted(std::monostate(), /*is_one_time=*/true);

  EXPECT_TRUE(allowed_.Get());
  EXPECT_TRUE(IsEnabled());
}

TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, DenyWithoutCheckbox) {
  auto request = CreateRequest();
  request->PermissionDenied();

  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled());
  EXPECT_TRUE(IsAskEnabled());
}

TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, DenyWithDontAskAgain) {
  auto request = CreateRequest();
  request->set_dont_ask_again(true);
  request->PermissionDenied();

  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled());
  EXPECT_FALSE(IsAskEnabled());
}

// The checkbox is about a block, so it means nothing when allowing.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, AllowIgnoresDontAskAgain) {
  auto request = CreateRequest();
  request->set_dont_ask_again(true);
  request->PermissionGranted(std::monostate(), /*is_one_time=*/false);

  EXPECT_TRUE(allowed_.Get());
  EXPECT_TRUE(IsEnabled());
  EXPECT_TRUE(IsAskEnabled());
}

// Dismissing saves nothing, the same as upstream permission prompts.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, DismissSavesNothing) {
  auto request = CreateRequest();
  request->set_dont_ask_again(true);
  request->Cancelled(/*is_final_decision=*/true);

  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled());
  EXPECT_TRUE(IsAskEnabled());
}

// A prompt that went away for now can be shown again, so it is not a decision.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest, NonFinalCancelWaits) {
  auto request = CreateRequest();
  request->Cancelled(/*is_final_decision=*/false);
  EXPECT_FALSE(allowed_.IsReady());

  request->PermissionGranted(std::monostate(), /*is_one_time=*/false);
  EXPECT_TRUE(allowed_.Get());
}

// A tab closing with the prompt open destroys the request without a decision,
// and whoever waits on it must still hear back.
TEST_F(OnDeviceSpeechModelPermissionRequestUnitTest,
       DestroyedWithoutADecisionDismisses) {
  auto request = CreateRequest();
  EXPECT_FALSE(allowed_.IsReady());
  request.reset();

  EXPECT_FALSE(allowed_.Get());
  EXPECT_FALSE(IsEnabled());
  EXPECT_TRUE(IsAskEnabled());
}

}  // namespace speech
