// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <Foundation/Foundation.h>

#include <memory>
#include <optional>
#include <string>

#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/gtest_util.h"
#include "base/values.h"
#include "brave/ios/web/js_messaging/js_prompt_communication_handler_impl.h"
#include "ios/web/js_messaging/java_script_feature_manager.h"
#include "ios/web/public/js_messaging/content_world.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/test/fakes/fake_navigation_context.h"
#include "ios/web/public/test/fakes/fake_web_state.h"
#include "ios/web/public/test/web_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "url/gurl.h"

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
    last_message_ = std::make_unique<ScriptMessage>(message);
    std::move(callback).Run(reply_value_.get(), /*error=*/nil);
  }

  void SetReply(std::unique_ptr<base::Value> value) {
    reply_value_ = std::move(value);
  }

  WebState* last_web_state() const { return last_web_state_; }
  const ScriptMessage* last_message() const { return last_message_.get(); }

 private:
  raw_ptr<WebState> last_web_state_ = nullptr;
  std::unique_ptr<ScriptMessage> last_message_;
  std::unique_ptr<base::Value> reply_value_;
};

NSString* BuildPromptJSON(NSString* handler, id message_object) {
  NSMutableDictionary* dict = [NSMutableDictionary dictionary];
  if (handler) {
    dict[@"handler"] = handler;
  }
  if (message_object) {
    dict[@"message"] = message_object;
  }
  NSData* data = [NSJSONSerialization dataWithJSONObject:dict
                                                 options:0
                                                   error:nil];
  return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

NSString* HandlerNameNS() {
  return base::SysUTF8ToNSString(kHandlerName);
}

}  // namespace

class JSPromptCommunicationHandlerImplTest : public WebTest {
 protected:
  void SetUp() override {
    WebTest::SetUp();

    feature_ =
        std::make_unique<TestPromptFeature>(ContentWorld::kPageContentWorld);
    JavaScriptFeatureManager::FromBrowserState(GetBrowserState())
        ->ConfigureFeatures({feature_.get()});

    web_state_ = std::make_unique<FakeWebState>();
    web_state_->SetBrowserState(GetBrowserState());

    handler_ = JSPromptCommunicationHandler::CreateJSPromptCommunicationHandler(
        web_state_.get());
  }

  void TearDown() override {
    handler_.reset();
    web_state_.reset();
    feature_.reset();
    WebTest::TearDown();
  }

  bool HandlePrompt(NSString* prompt, NSString** result) {
    return handler_->HandleJavaScriptPrompt(GURL("https://example.com/"),
                                            /*is_main_frame=*/true, prompt,
                                            result);
  }

  std::unique_ptr<TestPromptFeature> feature_;
  std::unique_ptr<FakeWebState> web_state_;
  std::unique_ptr<JSPromptCommunicationHandler> handler_;
};

// Tests that prompts whose body is not valid JSON are not handled.
TEST_F(JSPromptCommunicationHandlerImplTest, InvalidJsonReturnsFalse) {
  NSString* result = nil;
  EXPECT_FALSE(HandlePrompt(@"this is not json", &result));
  EXPECT_NSEQ(nil, result);
  EXPECT_EQ(nullptr, feature_->last_message());
}

// Tests that prompts whose body is valid JSON but not an object are not
// handled.
TEST_F(JSPromptCommunicationHandlerImplTest, NonObjectJsonReturnsFalse) {
  NSString* result = nil;
  EXPECT_FALSE(HandlePrompt(@"[1,2,3]", &result));
  EXPECT_NSEQ(nil, result);
}

// Tests that an object missing the "handler" key is not handled.
TEST_F(JSPromptCommunicationHandlerImplTest, MissingHandlerReturnsFalse) {
  NSString* prompt = BuildPromptJSON(/*handler=*/nil, @{});
  NSString* result = nil;
  EXPECT_FALSE(HandlePrompt(prompt, &result));
  EXPECT_NSEQ(nil, result);
}

// Tests that an object missing the "message" key is not handled.
TEST_F(JSPromptCommunicationHandlerImplTest, MissingMessageReturnsFalse) {
  NSString* prompt = BuildPromptJSON(HandlerNameNS(),
                                     /*message_object=*/nil);
  NSString* result = nil;
  EXPECT_FALSE(HandlePrompt(prompt, &result));
  EXPECT_NSEQ(nil, result);
}

// Tests that a prompt with a handler name that does not match any registered
// feature is not handled.
TEST_F(JSPromptCommunicationHandlerImplTest, UnknownHandlerReturnsFalse) {
  NSString* prompt = BuildPromptJSON(@"NonExistentHandler", @{});
  NSString* result = nil;
  EXPECT_FALSE(HandlePrompt(prompt, &result));
  EXPECT_NSEQ(nil, result);
  EXPECT_EQ(nullptr, feature_->last_message());
}

// Tests that a well-formed prompt is dispatched to the matching feature, and
// that the feature's synchronous reply is JSON-encoded into `result`.
TEST_F(JSPromptCommunicationHandlerImplTest, ValidPromptDispatchesToFeature) {
  feature_->SetReply(std::make_unique<base::Value>("hello"));

  NSString* prompt = BuildPromptJSON(HandlerNameNS(), @{@"key" : @"value"});
  NSString* result = nil;
  EXPECT_TRUE(handler_->HandleJavaScriptPrompt(GURL("https://example.com/foo"),
                                               /*is_main_frame=*/true, prompt,
                                               &result));
  EXPECT_NSEQ(@"\"hello\"", result);

  ASSERT_NE(nullptr, feature_->last_message());
  EXPECT_EQ(web_state_.get(), feature_->last_web_state());
  EXPECT_TRUE(feature_->last_message()->is_main_frame());
  EXPECT_FALSE(feature_->last_message()->is_user_interacting());
  ASSERT_TRUE(feature_->last_message()->request_url().has_value());
  EXPECT_EQ(GURL("https://example.com/foo"),
            *feature_->last_message()->request_url());

  base::Value* body = feature_->last_message()->body();
  ASSERT_TRUE(body);
  ASSERT_TRUE(body->is_dict());
  const std::string* key_value = body->GetDict().FindString("key");
  ASSERT_TRUE(key_value);
  EXPECT_EQ("value", *key_value);
}

// Tests that `is_main_frame=false` is propagated to the dispatched message.
TEST_F(JSPromptCommunicationHandlerImplTest, SubFramePropagatedToMessage) {
  feature_->SetReply(std::make_unique<base::Value>(0));

  NSString* prompt = BuildPromptJSON(HandlerNameNS(), @{});
  NSString* result = nil;
  EXPECT_TRUE(handler_->HandleJavaScriptPrompt(GURL("https://example.com/"),
                                               /*is_main_frame=*/false, prompt,
                                               &result));
  ASSERT_NE(nullptr, feature_->last_message());
  EXPECT_FALSE(feature_->last_message()->is_main_frame());
}

// Tests that when the feature's reply is null (e.g. the feature opted not to
// produce a value), the result is an empty string.
TEST_F(JSPromptCommunicationHandlerImplTest, NullReplyProducesEmptyString) {
  // No `SetReply` call: feature replies with a null base::Value*.
  NSString* prompt = BuildPromptJSON(HandlerNameNS(), @{});
  NSString* result = nil;
  EXPECT_TRUE(HandlePrompt(prompt, &result));
  EXPECT_NSEQ(@"", result);
}

// Tests that complex reply values are JSON-encoded into `result`.
TEST_F(JSPromptCommunicationHandlerImplTest, DictReplyIsJsonEncoded) {
  base::DictValue reply;
  reply.Set("ok", true);
  reply.Set("value", 42);
  feature_->SetReply(std::make_unique<base::Value>(std::move(reply)));

  NSString* prompt = BuildPromptJSON(HandlerNameNS(), @{});
  NSString* result = nil;
  EXPECT_TRUE(HandlePrompt(prompt, &result));

  std::optional<base::Value> parsed = base::JSONReader::Read(
      base::SysNSStringToUTF8(result), base::JSONParserOptions::JSON_PARSE_RFC);
  ASSERT_TRUE(parsed);
  ASSERT_TRUE(parsed->is_dict());
  EXPECT_EQ(true, parsed->GetDict().FindBool("ok"));
  EXPECT_EQ(42, parsed->GetDict().FindInt("value"));
}

// Tests that PageLoaded blocks subsequent prompts from being handled, matching
// the once-per-navigation contract documented in prompt_utils.ts.
TEST_F(JSPromptCommunicationHandlerImplTest,
       PageLoadedBlocksSubsequentPrompts) {
  web_state_->OnPageLoaded(PageLoadCompletionStatus::SUCCESS);

  NSString* prompt = BuildPromptJSON(HandlerNameNS(), @{});
  NSString* result = nil;
  EXPECT_FALSE(HandlePrompt(prompt, &result));
  EXPECT_NSEQ(nil, result);
  EXPECT_EQ(nullptr, feature_->last_message());
}

// Tests that DidStartNavigation re-enables prompt handling
TEST_F(JSPromptCommunicationHandlerImplTest, DidStartNavigationUnblocks) {
  feature_->SetReply(std::make_unique<base::Value>("ok"));

  NSString* prompt = BuildPromptJSON(HandlerNameNS(), @{});
  NSString* result = nil;
  ASSERT_TRUE(HandlePrompt(prompt, &result));

  web_state_->OnPageLoaded(PageLoadCompletionStatus::SUCCESS);
  ASSERT_FALSE(HandlePrompt(prompt, &result));

  FakeNavigationContext context;
  web_state_->OnNavigationStarted(&context);

  result = nil;
  EXPECT_TRUE(HandlePrompt(prompt, &result));
  EXPECT_NSEQ(@"\"ok\"", result);
}

}  // namespace web
