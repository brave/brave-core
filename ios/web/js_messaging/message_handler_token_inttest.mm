// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "base/test/ios/wait_util.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "brave/ios/web/js_messaging/randomized_message_handler_name.h"
#include "ios/web/public/js_messaging/content_world.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/test/fakes/fake_web_client.h"
#include "ios/web/public/test/js_test_util.h"
#include "ios/web/public/test/web_test_with_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Test feature that composes a RandomizedMessageHandlerName and records
// whether it has received a script message so tests can assert against it.
class TestJavaScriptFeature : public web::JavaScriptFeature {
 public:
  TestJavaScriptFeature()
      : JavaScriptFeature(
            web::ContentWorld::kPageContentWorld,
            {FeatureScript::CreateWithFilename(
                "tokenized_message_handler_test_api",
                FeatureScript::InjectionTime::kDocumentStart,
                FeatureScript::TargetFrames::kMainFrame,
                FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
                base::BindRepeating(
                    &web::MessageHandlerToken::GetPlaceholderReplacements,
                    base::Unretained(&token_)))}) {}
  ~TestJavaScriptFeature() override = default;

  std::optional<std::string> GetScriptMessageHandlerName() const override {
    return "ScriptHandlerName";
  }

  bool valid_message_received() const { return message_received_; }
  bool invalid_message_received() const { return invalid_message_received_; }

  void ScriptMessageReceived(web::WebState* web_state,
                             const web::ScriptMessage& message) override {
    auto body = token_.GetValidatedScriptMessageBody(message);
    if (body) {
      auto* dict = body.value()->GetIfDict();
      if (!dict) {
        return;
      }
      auto* value = dict->FindString("key");
      if (value) {
        message_received_ = *value == "value";
      }
    } else {
      invalid_message_received_ = true;
    }
  }

 private:
  web::MessageHandlerToken token_;
  bool message_received_ = false;
  bool invalid_message_received_ = false;
};

}  // namespace

class MessageHandlerTokenFeatureTest : public web::WebTestWithWebState {
 protected:
  MessageHandlerTokenFeatureTest()
      : web::WebTestWithWebState(std::make_unique<web::FakeWebClient>()) {}

  void SetUp() override {
    WebTestWithWebState::SetUp();

    feature_ = std::make_unique<TestJavaScriptFeature>();
    static_cast<web::FakeWebClient*>(GetWebClient())
        ->SetJavaScriptFeatures({feature_.get()});
    LoadHtml(@"<html></html>");
  }

  void TearDown() override {
    feature_.reset();
    WebTestWithWebState::TearDown();
  }

  std::unique_ptr<TestJavaScriptFeature> feature_;
};

// Tests that the message body received is valid with the correct token
TEST_F(MessageHandlerTokenFeatureTest, TestValidMessageReceived) {
  ASSERT_FALSE(feature_->valid_message_received());
  web::test::ExecuteJavaScriptForFeature(
      web_state(),
      @"__gCrWeb.callFunctionInGcrWeb('tokenized_message_handler_tests', "
      @"'sendTokenizedWebKitMessage', [])",
      feature_.get());
  ASSERT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        return feature_->valid_message_received();
      }));
}

// Tests that the message body received that contains no token is invalid and
// not handled
TEST_F(MessageHandlerTokenFeatureTest, TestInvalidMessageReceived) {
  ASSERT_FALSE(feature_->invalid_message_received());
  web::test::ExecuteJavaScriptForFeature(
      web_state(),
      @"__gCrWeb.callFunctionInGcrWeb('tokenized_message_handler_tests', "
      @"'sendWebKitMessage', [])",
      feature_.get());
  ASSERT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        return feature_->invalid_message_received();
      }));
}
