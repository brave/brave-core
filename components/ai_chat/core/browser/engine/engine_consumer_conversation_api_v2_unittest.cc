/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api_v2.h"

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_v2_client.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/extended_content_block.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using ::testing::_;

namespace ai_chat {

namespace {

constexpr int kTestingMaxAssociatedContentLength = 100;

struct GenerateRewriteTestParam {
  std::string name;
  mojom::ActionType action_type;
  ExtendedContentBlockType expected_content_type;
  std::string expected_payload;
  std::string expected_type_string;
};

}  // namespace

class MockConversationAPIV2Client : public ConversationAPIV2Client {
 public:
  explicit MockConversationAPIV2Client(const std::string& model_name)
      : ConversationAPIV2Client(model_name, nullptr, nullptr, nullptr) {}
  ~MockConversationAPIV2Client() override = default;

  MOCK_METHOD(void,
              PerformRequest,
              (std::vector<OAIMessage>,
               const std::string& selected_language,
               std::optional<base::Value::List> oai_tool_definitions,
               const std::optional<std::string>& preferred_tool_name,
               mojom::ConversationCapability conversation_capability,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback,
               const std::optional<std::string>& model_name),
              (override));

  std::string GetMessagesJson(std::vector<OAIMessage> messages) {
    auto body = CreateJSONRequestBody(
        std::move(messages), "", std::nullopt, std::nullopt,
        mojom::ConversationCapability::CHAT, std::nullopt, true);
    auto dict = base::test::ParseJsonDict(body);
    base::Value::List* messages_list = dict.FindList("messages");
    EXPECT_TRUE(messages_list);
    std::string messages_json;
    base::JSONWriter::WriteWithOptions(
        *messages_list, base::JSONWriter::OPTIONS_PRETTY_PRINT, &messages_json);
    return messages_json;
  }
};

class EngineConsumerConversationAPIV2UnitTest : public testing::Test {
 public:
  EngineConsumerConversationAPIV2UnitTest() = default;
  ~EngineConsumerConversationAPIV2UnitTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(prefs_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());
    model_service_ = std::make_unique<ModelService>(&prefs_);

    auto options = mojom::LeoModelOptions::New();
    options->display_maker = "Test Maker";
    options->name = "test-model-name";
    options->category = mojom::ModelCategory::CHAT;
    options->access = mojom::ModelAccess::BASIC;
    options->max_associated_content_length = kTestingMaxAssociatedContentLength;
    options->long_conversation_warning_character_limit = 1000;

    model_ = mojom::Model::New();
    model_->key = "test_model_key";
    model_->display_name = "Test Model Display Name";
    model_->options =
        mojom::ModelOptions::NewLeoModelOptions(std::move(options));

    engine_ = std::make_unique<EngineConsumerConversationAPIV2>(
        *model_->options->get_leo_model_options(), nullptr, nullptr,
        model_service_.get(), &prefs_);
    engine_->SetAPIForTesting(std::make_unique<MockConversationAPIV2Client>(
        model_->options->get_leo_model_options()->name));
  }

  MockConversationAPIV2Client* GetMockConversationAPIV2Client() {
    return static_cast<MockConversationAPIV2Client*>(
        engine_->GetAPIForTesting());
  }

  void TearDown() override {}

  std::string FormatComparableMessagesJson(std::string_view formatted_json) {
    auto messages = base::test::ParseJson(formatted_json);
    std::string messages_json;
    base::JSONWriter::WriteWithOptions(
        messages, base::JSONWriter::OPTIONS_PRETTY_PRINT, &messages_json);
    return messages_json;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  mojom::ModelPtr model_;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<EngineConsumerConversationAPIV2> engine_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateRewriteSuggestion_UnsupportedActionTypeReturnsInternalError) {
  auto* client = GetMockConversationAPIV2Client();

  // Expect PerformRequest is NOT called for unsupported action types
  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _, _, _, _)).Times(0);

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateRewriteSuggestion("Hello World",
                                     mojom::ActionType::CREATE_TAGLINE, "",
                                     base::DoNothing(), future.GetCallback());

  auto result = future.Take();
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);

  testing::Mock::VerifyAndClearExpectations(client);
}

class EngineConsumerConversationAPIV2UnitTest_GenerateRewrite
    : public EngineConsumerConversationAPIV2UnitTest,
      public testing::WithParamInterface<GenerateRewriteTestParam> {};

TEST_P(EngineConsumerConversationAPIV2UnitTest_GenerateRewrite,
       GenerateRewriteSuggestion) {
  GenerateRewriteTestParam params = GetParam();
  auto* client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;

  std::string test_text = "Hello World";
  std::string expected_response = "Rewritten text here.";

  // Build expected JSON format
  std::string expected_messages;
  if (params.expected_content_type == ExtendedContentBlockType::kChangeTone) {
    expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {"type": "brave-page-excerpt", "text": "%s"},
              {"type": "%s", "text": "", "tone": "%s"}
            ]
          }
        ])",
        test_text, params.expected_type_string, params.expected_payload);
  } else {
    expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {"type": "brave-page-excerpt", "text": "%s"},
              {"type": "%s", "text": ""}
            ]
          }
        ])",
        test_text, params.expected_type_string);
  }

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _, _, _, _))
      .WillOnce(
          [&](std::vector<OAIMessage> messages,
              const std::string& selected_language,
              std::optional<base::Value::List> oai_tool_definitions,
              const std::optional<std::string>& preferred_tool_name,
              mojom::ConversationCapability conversation_capability,
              EngineConsumer::GenerationDataCallback data_callback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::string>& model_name) {
            // Verify conversation capability is CHAT
            EXPECT_EQ(conversation_capability,
                      mojom::ConversationCapability::CHAT);

            // Verify no tool definitions for rewrite requests
            EXPECT_FALSE(oai_tool_definitions.has_value());
            EXPECT_FALSE(preferred_tool_name.has_value());

            // Verify messages structure
            ASSERT_GE(messages.size(), 1u);

            // First message should contain the text and action content block
            const auto& first_message = messages[0];
            EXPECT_EQ(first_message.role, "user");
            ASSERT_GE(first_message.content.size(), 2u);

            // First content block should be the page excerpt
            EXPECT_EQ(first_message.content[0].type,
                      ExtendedContentBlockType::kPageExcerpt);
            EXPECT_EQ(std::get<TextContent>(first_message.content[0].data).text,
                      test_text);

            // Second content block should be the action type
            EXPECT_EQ(first_message.content[1].type,
                      params.expected_content_type);

            // Verify the content data, should have tone for change tone type,
            // empty text otherwise.
            if (params.expected_content_type ==
                ExtendedContentBlockType::kChangeTone) {
              auto* tone_content = std::get_if<ChangeToneContent>(
                  &first_message.content[1].data);
              ASSERT_TRUE(tone_content);
              EXPECT_EQ(tone_content->tone, params.expected_payload);
            } else {
              auto* text_content =
                  std::get_if<TextContent>(&first_message.content[1].data);
              ASSERT_TRUE(text_content);
              EXPECT_EQ(text_content->text, params.expected_payload);
            }

            // Verify JSON serialization matches expected format
            EXPECT_EQ(client->GetMessagesJson(std::move(messages)),
                      FormatComparableMessagesJson(expected_messages));

            // Return completion
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(expected_response)),
                    std::nullopt)));
          });

  engine_->GenerateRewriteSuggestion(
      test_text, params.action_type, "", base::DoNothing(),
      base::BindLambdaForTesting([&run_loop, &expected_response](
                                     EngineConsumer::GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->event);
        ASSERT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion,
                  expected_response);
        run_loop.Quit();
      }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    EngineConsumerConversationAPIV2UnitTest_GenerateRewrite,
    testing::Values(
        GenerateRewriteTestParam{"Paraphrase", mojom::ActionType::PARAPHRASE,
                                 ExtendedContentBlockType::kParaphrase, "",
                                 "brave-request-paraphrase"},
        GenerateRewriteTestParam{"Improve", mojom::ActionType::IMPROVE,
                                 ExtendedContentBlockType::kImprove, "",
                                 "brave-request-improve-excerpt-language"},
        GenerateRewriteTestParam{"Shorten", mojom::ActionType::SHORTEN,
                                 ExtendedContentBlockType::kShorten, "",
                                 "brave-request-shorten"},
        GenerateRewriteTestParam{"Expand", mojom::ActionType::EXPAND,
                                 ExtendedContentBlockType::kExpand, "",
                                 "brave-request-expansion"},
        GenerateRewriteTestParam{"Academic", mojom::ActionType::ACADEMICIZE,
                                 ExtendedContentBlockType::kChangeTone,
                                 "academic", "brave-request-change-tone"},
        GenerateRewriteTestParam{"Professional",
                                 mojom::ActionType::PROFESSIONALIZE,
                                 ExtendedContentBlockType::kChangeTone,
                                 "professional", "brave-request-change-tone"},
        GenerateRewriteTestParam{"Casual", mojom::ActionType::CASUALIZE,
                                 ExtendedContentBlockType::kChangeTone,
                                 "casual", "brave-request-change-tone"},
        GenerateRewriteTestParam{"Funny", mojom::ActionType::FUNNY_TONE,
                                 ExtendedContentBlockType::kChangeTone, "funny",
                                 "brave-request-change-tone"},
        GenerateRewriteTestParam{"Persuasive",
                                 mojom::ActionType::PERSUASIVE_TONE,
                                 ExtendedContentBlockType::kChangeTone,
                                 "persuasive", "brave-request-change-tone"}),
    [](const testing::TestParamInfo<GenerateRewriteTestParam>& info) {
      return info.param.name;
    });

}  // namespace ai_chat
