// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/js_messaging/prompt_facade.h"

#include <Foundation/Foundation.h>

#include <memory>
#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/test/gtest_util.h"
#include "base/values.h"
#include "ios/web/js_messaging/java_script_feature_manager.h"
#include "ios/web/public/js_messaging/content_world.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/test/fakes/fake_web_state.h"
#include "ios/web/public/test/web_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace web {

namespace {

constexpr char kHandlerName[] = "TestPromptHandler";

// Test feature that opts in to replying to window.prompt() messages and
// captures the most recent message it receives so tests can assert against it.
class TestPromptFeature : public JavaScriptFeature {
 public:
  explicit TestPromptFeature(ContentWorld content_world)
      : JavaScriptFeature(content_world) {}
  ~TestPromptFeature() override = default;

  std::optional<std::string> GetScriptMessageHandlerName() const override {
    return std::string(kHandlerName);
  }

  bool GetFeatureRepliesToPrompts() const override { return true; }

  void ScriptMessageReceivedWithReply(
      WebState* web_state,
      const ScriptMessage& message,
      ScriptMessageReplyCallback callback) override {
    last_web_state_ = web_state;
    last_message_ = std::make_unique<ScriptMessage>(
        message.legacy_body()
            ? std::make_unique<base::Value>(message.legacy_body()->Clone())
            : nullptr,
        message.is_user_interacting(), message.is_main_frame(),
        message.request_url(), message.security_origin());
    std::move(callback).Run(reply_value_.get(), /*error=*/nil);
  }

  void SetReply(std::unique_ptr<base::Value> value) {
    reply_value_ = std::move(value);
  }

  WebState* last_web_state() const { return last_web_state_; }
  const ScriptMessage* last_message() const { return last_message_.get(); }

 private:
  std::unique_ptr<ScriptMessage> last_message_;
  std::unique_ptr<base::Value> reply_value_;
  raw_ptr<WebState> last_web_state_ = nullptr;
};

std::string BuildPromptJSON(std::optional<std::string> handler,
                            std::optional<base::DictValue> message) {
  base::DictValue dict;
  if (handler) {
    dict.Set("handler", *handler);
  }
  if (message) {
    dict.Set("message", std::move(*message));
  }
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

}  // namespace

class PromptFacadeTest : public WebTest {
 protected:
  void SetUp() override {
    WebTest::SetUp();

    feature_ =
        std::make_unique<TestPromptFeature>(ContentWorld::kPageContentWorld);
    JavaScriptFeatureManager::FromBrowserState(GetBrowserState())
        ->ConfigureFeatures({feature_.get()});

    web_state_ = std::make_unique<FakeWebState>();
    web_state_->SetBrowserState(GetBrowserState());

    handler_ = std::make_unique<web::PromptFacade>(web_state_.get());
  }

  void TearDown() override {
    handler_.reset();
    web_state_.reset();
    feature_.reset();
    WebTest::TearDown();
  }

  // Dispatches `prompt` and returns the JSON-encoded reply if a feature handled
  // it, or std::nullopt if it was not handled. The reply is captured
  // synchronously because the test feature invokes the callback inline.
  std::optional<std::string> HandlePrompt(const std::string& prompt) {
    GURL url("https://example.com/");
    return HandlePrompt(url, url::Origin::Create(url),
                        /*is_main_frame=*/true, prompt);
  }

  std::optional<std::string> HandlePrompt(const GURL& url,
                                          const url::Origin& origin,
                                          bool is_main_frame,
                                          const std::string& prompt) {
    // For testing purposes, the JS feature will always callback synchronously
    std::optional<std::string> reply;
    bool handled = handler_->HandleJavaScriptPrompt(
        url, origin, is_main_frame, prompt,
        base::BindOnce(
            [](std::optional<std::string>* reply, std::string response) {
              *reply = std::move(response);
            },
            &reply));
    if (!handled) {
      return std::nullopt;
    }
    return reply;
  }

  std::unique_ptr<TestPromptFeature> feature_;
  std::unique_ptr<FakeWebState> web_state_;
  std::unique_ptr<PromptFacade> handler_;
};

// Tests that prompts whose body is not valid JSON are not handled.
TEST_F(PromptFacadeTest, InvalidJsonReturnsNullopt) {
  EXPECT_EQ(std::nullopt, HandlePrompt("this is not json"));
  EXPECT_EQ(nullptr, feature_->last_message());
}

// Tests that prompts whose body is valid JSON but not an object are not
// handled.
TEST_F(PromptFacadeTest, NonObjectJsonReturnsNullopt) {
  EXPECT_EQ(std::nullopt, HandlePrompt("[1,2,3]"));
}

// Tests that an object missing the "handler" key is not handled.
TEST_F(PromptFacadeTest, MissingHandlerReturnsNullopt) {
  std::string prompt =
      BuildPromptJSON(/*handler=*/std::nullopt, base::DictValue());
  EXPECT_EQ(std::nullopt, HandlePrompt(prompt));
}

// Tests that an object missing the "message" key is not handled.
TEST_F(PromptFacadeTest, MissingMessageReturnsNullopt) {
  std::string prompt = BuildPromptJSON(kHandlerName,
                                       /*message=*/std::nullopt);
  EXPECT_EQ(std::nullopt, HandlePrompt(prompt));
}

// Tests that a prompt with a handler name that does not match any registered
// feature is not handled.
TEST_F(PromptFacadeTest, UnknownHandlerReturnsNullopt) {
  std::string prompt = BuildPromptJSON("NonExistentHandler", base::DictValue());
  EXPECT_EQ(std::nullopt, HandlePrompt(prompt));
  EXPECT_EQ(nullptr, feature_->last_message());
}

// Tests that a well-formed prompt is dispatched to the matching feature, and
// that the feature's synchronous reply is JSON-encoded into `result`.
TEST_F(PromptFacadeTest, ValidPromptDispatchesToFeature) {
  feature_->SetReply(std::make_unique<base::Value>("hello"));

  base::DictValue message;
  message.Set("key", "value");
  std::string prompt = BuildPromptJSON(kHandlerName, std::move(message));
  GURL url("https://example.com/foo");
  std::optional<std::string> result = HandlePrompt(
      url, url::Origin::Create(url), /*is_main_frame=*/true, prompt);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ("\"hello\"", *result);

  ASSERT_NE(nullptr, feature_->last_message());
  EXPECT_EQ(web_state_.get(), feature_->last_web_state());
  EXPECT_TRUE(feature_->last_message()->is_main_frame());
  EXPECT_FALSE(feature_->last_message()->is_user_interacting());
  ASSERT_TRUE(feature_->last_message()->request_url().has_value());
  EXPECT_EQ(GURL("https://example.com/foo"),
            *feature_->last_message()->request_url());

  base::Value* body = feature_->last_message()->legacy_body();
  ASSERT_TRUE(body);
  ASSERT_TRUE(body->is_dict());
  const std::string* key_value = body->GetDict().FindString("key");
  ASSERT_TRUE(key_value);
  EXPECT_EQ("value", *key_value);
}

// Tests that `is_main_frame=false` is propagated to the dispatched message.
TEST_F(PromptFacadeTest, SubFramePropagatedToMessage) {
  feature_->SetReply(std::make_unique<base::Value>("hello"));

  std::string prompt = BuildPromptJSON(kHandlerName, base::DictValue());
  GURL url("https://example.com/");
  std::optional<std::string> result = HandlePrompt(
      url, url::Origin::Create(url), /*is_main_frame=*/false, prompt);
  EXPECT_TRUE(result.has_value());
  ASSERT_NE(nullptr, feature_->last_message());
  EXPECT_FALSE(feature_->last_message()->is_main_frame());
}

// Tests that when the feature's reply is null (e.g. the feature opted not to
// produce a value), the result is an empty string.
TEST_F(PromptFacadeTest, NullReplyProducesEmptyString) {
  // No `SetReply` call: feature replies with a null base::Value*.
  std::string prompt = BuildPromptJSON(kHandlerName, base::DictValue());
  std::optional<std::string> result = HandlePrompt(prompt);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ("", *result);
}

// Tests that complex reply values are JSON-encoded into `result`.
TEST_F(PromptFacadeTest, DictReplyIsJsonEncoded) {
  base::DictValue reply;
  reply.Set("ok", true);
  reply.Set("value", 42);
  feature_->SetReply(std::make_unique<base::Value>(std::move(reply)));

  std::string prompt = BuildPromptJSON(kHandlerName, base::DictValue());
  std::optional<std::string> result = HandlePrompt(prompt);
  ASSERT_TRUE(result.has_value());

  std::optional<base::Value> parsed =
      base::JSONReader::Read(*result, base::JSONParserOptions::JSON_PARSE_RFC);
  ASSERT_TRUE(parsed);
  ASSERT_TRUE(parsed->is_dict());
  EXPECT_EQ(true, parsed->GetDict().FindBool("ok"));
  EXPECT_EQ(42, parsed->GetDict().FindInt("value"));
}

}  // namespace web
