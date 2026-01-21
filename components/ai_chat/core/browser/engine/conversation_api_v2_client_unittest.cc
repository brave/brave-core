/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/conversation_api_v2_client.h"

#include <optional>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/test_utils.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/api_request_helper/mock_api_request_helper.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using ::testing::_;
using ::testing::Sequence;
using DataReceivedCallback =
    api_request_helper::APIRequestHelper::DataReceivedCallback;
using ResultCallback = api_request_helper::APIRequestHelper::ResultCallback;
using Ticket = api_request_helper::APIRequestHelper::Ticket;
using api_request_helper::MockAPIRequestHelper;

namespace ai_chat {

namespace {

constexpr char kTestContent[] = "test content";

struct ContentBlockTestParam {
  std::string name;
  base::RepeatingCallback<mojom::ContentBlockPtr()> get_test_content_block;
  std::string expected_json;
};

std::pair<std::vector<OAIMessage>, base::Value::List>
GetMockMessagesAndExpectedMessagesJson() {
  std::vector<OAIMessage> messages;

  // User message with multiple content blocks
  {
    OAIMessage message;
    message.role = "user";

    // Memory block
    auto memory_map = BuildExpectedMemory(
        {{"name", "Jane"}}, {{"memories", {"memory1", "memory2"}}});
    message.content.push_back(mojom::ContentBlock::NewMemoryContentBlock(
        mojom::MemoryContentBlock::New(std::move(memory_map))));

    // Page text block
    message.content.push_back(mojom::ContentBlock::NewPageTextContentBlock(
        mojom::PageTextContentBlock::New(
            "This is a page about The Mandalorian.")));

    // Page excerpt block
    message.content.push_back(mojom::ContentBlock::NewPageExcerptContentBlock(
        mojom::PageExcerptContentBlock::New("The Mandalorian")));

    // Text block
    message.content.push_back(mojom::ContentBlock::NewTextContentBlock(
        mojom::TextContentBlock::New("Est-ce lié à une série plus large?")));

    messages.push_back(std::move(message));
  }

  // Assistant message with tool calls
  {
    OAIMessage message;
    message.role = "assistant";
    message.content.push_back(mojom::ContentBlock::NewTextContentBlock(
        mojom::TextContentBlock::New("Going to use a tool...")));
    message.tool_calls.push_back(mojom::ToolUseEvent::New(
        "get_weather", "123", "{\"location\":\"New York\"}", std::nullopt,
        std::nullopt, nullptr));
    message.tool_calls.push_back(
        mojom::ToolUseEvent::New("get_screenshot", "456", "{\"type\":\"tab\"}",
                                 std::nullopt, std::nullopt, nullptr));
    messages.push_back(std::move(message));
  }

  // First tool response
  {
    OAIMessage message;
    message.role = "tool";
    message.tool_call_id = "123";
    message.content.push_back(
        mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
            "The temperature in New York is 60 degrees.")));
    message.content.push_back(
        mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
            "The wind in New York is 5 mph from the SW.")));
    messages.push_back(std::move(message));
  }

  // Second tool response
  {
    OAIMessage message;
    message.role = "tool";
    message.tool_call_id = "456";
    message.content.push_back(
        mojom::ContentBlock::NewImageContentBlock(mojom::ImageContentBlock::New(
            GURL("data:image/png;base64,R0lGODlhAQABAIAAAAAAAP///"
                 "yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"))));
    messages.push_back(std::move(message));
  }

  const std::string expected_messages_body = R"([
    {
      "role": "user",
      "content": [
        {
          "type": "brave-user-memory",
          "memory": {"name": "Jane", "memories": ["memory1", "memory2"]}
        },
        {
          "type": "brave-page-text",
          "text": "This is a page about The Mandalorian."
        },
        {
          "type": "brave-page-excerpt",
          "text": "The Mandalorian"
        },
        {
          "type": "text",
          "text": "Est-ce lié à une série plus large?"
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "Going to use a tool..."
        }
      ],
      "tool_calls": [
        {
          "id": "123",
          "type": "function",
          "function": {
            "name": "get_weather",
            "arguments": "{\"location\":\"New York\"}"
          }
        },
        {
          "id": "456",
          "type": "function",
          "function": {
            "name": "get_screenshot",
            "arguments": "{\"type\":\"tab\"}"
          }
        }
      ]
    },
    {
      "role": "tool",
      "tool_call_id": "123",
      "content": [
        {
          "type": "text",
          "text": "The temperature in New York is 60 degrees."
        },
        {
          "type": "text",
          "text": "The wind in New York is 5 mph from the SW."
        }
      ]
    },
    {
      "role": "tool",
      "tool_call_id": "456",
      "content": [
        {
          "type": "image_url",
          "image_url": {
            "url": "data:image/png;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"
          }
        }
      ]
    }
  ])";

  return std::make_pair(std::move(messages),
                        base::test::ParseJsonList(expected_messages_body));
}

class MockCallbacks {
 public:
  MOCK_METHOD(void, OnDataReceived, (EngineConsumer::GenerationResultData));
  MOCK_METHOD(void, OnCompleted, (EngineConsumer::GenerationResult));
};

}  // namespace

// Mock credential manager to control premium/non-premium behavior
class MockAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;
  ~MockAIChatCredentialManager() override = default;

  MOCK_METHOD(void,
              FetchPremiumCredential,
              (base::OnceCallback<void(std::optional<CredentialCacheEntry>)>),
              (override));
};

// Create a version of the ConversationAPIClient that contains our mocks
class TestConversationAPIV2Client : public ConversationAPIV2Client {
 public:
  TestConversationAPIV2Client(AIChatCredentialManager* credential_manager,
                              ModelService* model_service)
      : ConversationAPIV2Client("test-model-name",
                                nullptr,
                                credential_manager,
                                model_service) {
    auto mock_helper =
        std::make_unique<testing::NiceMock<MockAPIRequestHelper>>(
            TRAFFIC_ANNOTATION_FOR_TESTS, nullptr);
    SetAPIRequestHelperForTesting(std::move(mock_helper));
  }

  MockAPIRequestHelper* GetMockAPIRequestHelper() {
    return static_cast<MockAPIRequestHelper*>(GetAPIRequestHelperForTesting());
  }
};

class ConversationAPIV2ClientUnitTest : public testing::Test {
 public:
  ConversationAPIV2ClientUnitTest() = default;
  ~ConversationAPIV2ClientUnitTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(prefs_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());
    credential_manager_ = std::make_unique<MockAIChatCredentialManager>(
        base::NullCallback(), &prefs_);
    model_service_ = std::make_unique<ModelService>(&prefs_);

    client_ = std::make_unique<TestConversationAPIV2Client>(
        credential_manager_.get(), model_service_.get());

    // Default credential behavior: non-premium (nullopt)
    ON_CALL(*credential_manager_, FetchPremiumCredential(_))
        .WillByDefault(
            [&](base::OnceCallback<void(std::optional<CredentialCacheEntry>)>
                    callback) { std::move(callback).Run(credential_); });
  }

  void TearDown() override {}

  // Returns a pair of system_language and selected_langauge
  // The system language is the OS locale.
  // The selected language is the language the server side determined the
  // conversation is in
  std::pair<std::string, std::optional<std::string>> GetLanguage(
      const base::Value::Dict& body) {
    const std::string* system_language = body.FindString("system_language");
    // The system language should always be present
    EXPECT_TRUE(system_language != nullptr);

    const std::string* selected_language = body.FindString("selected_language");
    if (selected_language) {
      return {*system_language, *selected_language};
    } else {
      return {*system_language, std::nullopt};
    }
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<MockAIChatCredentialManager> credential_manager_;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<TestConversationAPIV2Client> client_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::optional<CredentialCacheEntry> credential_ = std::nullopt;
};

class ConversationAPIV2ClientUnitTest_ContentBlocks
    : public ConversationAPIV2ClientUnitTest,
      public testing::WithParamInterface<ContentBlockTestParam> {};

TEST_P(ConversationAPIV2ClientUnitTest_ContentBlocks,
       SerializeOAIMessages_ContentBlocks) {
  ContentBlockTestParam params = GetParam();

  // Create message with content block
  mojom::ContentBlockPtr block = params.get_test_content_block.Run();
  std::vector<OAIMessage> messages;
  OAIMessage message;
  message.role = "user";
  message.content.emplace_back(std::move(block));
  messages.push_back(std::move(message));

  base::Value::List serialized =
      ConversationAPIV2Client::SerializeOAIMessages(std::move(messages));
  std::string expected_json = absl::StrFormat(
      R"([{"role": "user", "content": [%s]}])", params.expected_json);

  // Compare using IsJson matcher
  EXPECT_THAT(serialized, base::test::IsJson(expected_json));
}

INSTANTIATE_TEST_SUITE_P(
    ,
    ConversationAPIV2ClientUnitTest_ContentBlocks,
    testing::Values(
        ContentBlockTestParam{"Text", base::BindRepeating([]() {
                                return mojom::ContentBlock::NewTextContentBlock(
                                    mojom::TextContentBlock::New(kTestContent));
                              }),
                              R"({"type": "text", "text": "test content"})"},
        ContentBlockTestParam{
            "Image", base::BindRepeating([]() {
              return mojom::ContentBlock::NewImageContentBlock(
                  mojom::ImageContentBlock::New(
                      GURL("data:image/png;base64,abc123")));
            }),
            R"({
              "type": "image_url",
              "image_url": {"url": "data:image/png;base64,abc123"}
            })"},
        ContentBlockTestParam{
            "File", base::BindRepeating([]() {
              return mojom::ContentBlock::NewFileContentBlock(
                  mojom::FileContentBlock::New(
                      GURL("data:application/pdf;base64,abc123"), "filename"));
            }),
            R"({
              "type": "file",
              "file": {
                "filename": "filename",
                "file_data": "data:application/pdf;base64,abc123"
              }
            })"},
        ContentBlockTestParam{
            "PageExcerpt", base::BindRepeating([]() {
              return mojom::ContentBlock::NewPageExcerptContentBlock(
                  mojom::PageExcerptContentBlock::New(kTestContent));
            }),
            R"({"type": "brave-page-excerpt", "text": "test content"})"},
        ContentBlockTestParam{
            "PageText", base::BindRepeating([]() {
              return mojom::ContentBlock::NewPageTextContentBlock(
                  mojom::PageTextContentBlock::New("test page content"));
            }),
            R"({"type": "brave-page-text", "text": "test page content"})"},
        ContentBlockTestParam{
            "VideoTranscript", base::BindRepeating([]() {
              return mojom::ContentBlock::NewVideoTranscriptContentBlock(
                  mojom::VideoTranscriptContentBlock::New(
                      "test video transcript"));
            }),
            R"({
              "type": "brave-video-transcript",
              "text": "test video transcript"
            })"},
        ContentBlockTestParam{
            "SimpleRequest_Paraphrase", base::BindRepeating([]() {
              return mojom::ContentBlock::NewSimpleRequestContentBlock(
                  mojom::SimpleRequestContentBlock::New(
                      mojom::SimpleRequestType::kParaphrase));
            }),
            R"({"type": "brave-request-paraphrase", "text": ""})"},
        ContentBlockTestParam{
            "SimpleRequest_Improve", base::BindRepeating([]() {
              return mojom::ContentBlock::NewSimpleRequestContentBlock(
                  mojom::SimpleRequestContentBlock::New(
                      mojom::SimpleRequestType::kImprove));
            }),
            R"({
              "type": "brave-request-improve-excerpt-language",
              "text": ""
            })"},
        ContentBlockTestParam{
            "SimpleRequest_Shorten", base::BindRepeating([]() {
              return mojom::ContentBlock::NewSimpleRequestContentBlock(
                  mojom::SimpleRequestContentBlock::New(
                      mojom::SimpleRequestType::kShorten));
            }),
            R"({"type": "brave-request-shorten", "text": ""})"},
        ContentBlockTestParam{
            "SimpleRequest_Expand", base::BindRepeating([]() {
              return mojom::ContentBlock::NewSimpleRequestContentBlock(
                  mojom::SimpleRequestContentBlock::New(
                      mojom::SimpleRequestType::kExpand));
            }),
            R"({"type": "brave-request-expansion", "text": ""})"},
        ContentBlockTestParam{
            "SimpleRequest_RequestSummary", base::BindRepeating([]() {
              return mojom::ContentBlock::NewSimpleRequestContentBlock(
                  mojom::SimpleRequestContentBlock::New(
                      mojom::SimpleRequestType::kRequestSummary));
            }),
            R"({"type": "brave-request-summary", "text": ""})"},
        ContentBlockTestParam{
            "SimpleRequest_RequestQuestions", base::BindRepeating([]() {
              return mojom::ContentBlock::NewSimpleRequestContentBlock(
                  mojom::SimpleRequestContentBlock::New(
                      mojom::SimpleRequestType::kRequestQuestions));
            }),
            R"({"type": "brave-request-questions", "text": ""})"},
        ContentBlockTestParam{
            "RequestTitle", base::BindRepeating([]() {
              return mojom::ContentBlock::NewRequestTitleContentBlock(
                  mojom::RequestTitleContentBlock::New(kTestContent));
            }),
            R"({
              "type": "brave-conversation-title",
              "text": "test content"
            })"},
        ContentBlockTestParam{
            "ChangeTone", base::BindRepeating([]() {
              return mojom::ContentBlock::NewChangeToneContentBlock(
                  mojom::ChangeToneContentBlock::New("", "professional"));
            }),
            R"({
              "type": "brave-request-change-tone",
              "text": "",
              "tone": "professional"
            })"},
        ContentBlockTestParam{
            "Memory", base::BindRepeating([]() {
              base::flat_map<std::string, mojom::MemoryValuePtr> memory;
              memory["job"] =
                  mojom::MemoryValue::NewStringValue("software engineer");
              std::vector<std::string> memories = {"mem1", "mem2"};
              memory["memories"] = mojom::MemoryValue::NewListValue(memories);
              return mojom::ContentBlock::NewMemoryContentBlock(
                  mojom::MemoryContentBlock::New(std::move(memory)));
            }),
            R"({
              "type": "brave-user-memory",
              "memory": {
                "job": "software engineer",
                "memories": ["mem1", "mem2"]
              }
            })"},
        ContentBlockTestParam{
            "SuggestFocusTopics", base::BindRepeating([]() {
              return mojom::ContentBlock::NewSuggestFocusTopicsContentBlock(
                  mojom::SuggestFocusTopicsContentBlock::New(
                      R"([{"id":"1","title":"Tab 1"}])"));
            }),
            R"({
              "type": "brave-suggest-focus-topics",
              "text": "[{\"id\":\"1\",\"title\":\"Tab 1\"}]"
            })"},
        ContentBlockTestParam{
            "SuggestFocusTopicsWithEmoji", base::BindRepeating([]() {
              return mojom::ContentBlock::
                  NewSuggestFocusTopicsWithEmojiContentBlock(
                      mojom::SuggestFocusTopicsWithEmojiContentBlock::New(
                          R"([{"id":"2","title":"Tab 2"}])"));
            }),
            R"({
              "type": "brave-suggest-focus-topics-emoji",
              "text": "[{\"id\":\"2\",\"title\":\"Tab 2\"}]"
            })"},
        ContentBlockTestParam{
            "FilterTabs", base::BindRepeating([]() {
              return mojom::ContentBlock::NewFilterTabsContentBlock(
                  mojom::FilterTabsContentBlock::New(
                      R"([{"id":"3","title":"Shopping"}])", "Shopping"));
            }),
            R"({
              "type": "brave-filter-tabs",
              "text": "[{\"id\":\"3\",\"title\":\"Shopping\"}]",
              "topic": "Shopping"
            })"},
        ContentBlockTestParam{
            "ReduceFocusTopics", base::BindRepeating([]() {
              return mojom::ContentBlock::NewReduceFocusTopicsContentBlock(
                  mojom::ReduceFocusTopicsContentBlock::New(
                      R"(["Shopping","News"])"));
            }),
            R"({
              "type": "brave-reduce-focus-topics",
              "text": "[\"Shopping\",\"News\"]"
            })"}),
    [](const testing::TestParamInfo<ContentBlockTestParam>& info) {
      return info.param.name;
    });

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_PremiumHeaders) {
  // Tests the request building part of the ConversationAPIClient:
  //  - headers are set correctly when premium credentials are available
  //  - messages are correctly formatted into JSON
  //  - completion response is parsed and passed through to the callbacks
  std::string expected_credential = "test-premium-credential";
  auto [messages, expected_messages_json] =
      GetMockMessagesAndExpectedMessagesJson();
  std::string expected_system_language = "en_KY";
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale(
      expected_system_language);
  std::string expected_completion_response = "premium response";
  std::string expected_selected_language = "fr";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  // Intercept credential fetch and provide premium credentials
  credential_ = CredentialCacheEntry{expected_credential,
                                     base::Time::Now() + base::Hours(1)};

  // Intercept API Request Helper call and verify the request is as expected
  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .WillOnce([&](const std::string& method, const GURL& url,
                    const std::string& body, const std::string& content_type,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback,
                    const base::flat_map<std::string, std::string>& headers,
                    const api_request_helper::APIRequestOptions& options) {
        EXPECT_TRUE(url.is_valid());
        EXPECT_TRUE(url.SchemeIs(url::kHttpsScheme));
        EXPECT_FALSE(body.empty());
        EXPECT_EQ(net::HttpRequestHeaders::kPostMethod, method);

        // Verify premium Cookie header is present
        EXPECT_TRUE(headers.contains("Cookie"));
        const auto& cookie = headers.at("Cookie");
        EXPECT_EQ(cookie,
                  "__Secure-sku#brave-leo-premium=" + expected_credential);

        // Verify other headers still present
        EXPECT_TRUE(headers.contains("x-brave-key"));
        EXPECT_TRUE(headers.contains("digest"));
        EXPECT_TRUE(headers.contains(net::HttpRequestHeaders::kAuthorization));

        // Verify body
        auto body_dict = base::test::ParseJsonDict(body);
        EXPECT_TRUE(!body_dict.empty());

        // Verify body contains the language
        auto [system_language, selected_language] = GetLanguage(body_dict);
        EXPECT_EQ(system_language, expected_system_language);
        EXPECT_TRUE(selected_language.has_value());
        EXPECT_TRUE(selected_language.value().empty());

        // Currently server only expects we pass content_agent capability,
        // so it won't be passed for CHAT.
        EXPECT_FALSE(body_dict.FindString("brave_capability"));

        // Verify body contains the stream
        std::optional<bool> stream = body_dict.FindBool("stream");
        EXPECT_TRUE(stream);
        EXPECT_TRUE(*stream);

        // Verify messages content matches expected
        const base::Value::List* messages_list = body_dict.FindList("messages");
        EXPECT_TRUE(messages_list);
        EXPECT_EQ(*messages_list, expected_messages_json);

        // Simulate streaming chunk
        auto chunk_dict = base::test::ParseJsonDict(R"({
          "model": "chat-claude-sonnet",
          "choices": [{
            "delta": {"content": "chunk text"}
          }]
        })");
        data_received_callback.Run(
            base::ok(base::Value(std::move(chunk_dict))));

        // Simulate completion
        auto completion_dict = base::test::ParseJsonDict(R"({
          "model": "chat-claude-sonnet",
          "choices": [{
            "message": {"content": "premium response"}
          }]
        })");
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(
                200, base::Value(std::move(completion_dict)), {}, net::OK,
                GURL()));

        run_loop.Quit();
        return Ticket();
      });

  // Callbacks should be passed through and translated from APIRequestHelper
  // format.
  Sequence seq;
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_completion_event());
        EXPECT_EQ(result.event->get_completion_event()->completion,
                  "chunk text");
      });
  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](EngineConsumer::GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result->event);
        EXPECT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion,
                  expected_completion_response);
      });

  // Begin request
  client_->PerformRequest(
      std::move(messages), "" /* selected_language */, std::nullopt,
      /* oai_tool_definitions */ std::nullopt, /* preferred_tool_name */
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_NonPremium) {
  // Performs the same test as Premium, verifying that nothing else changes
  // apart from request headers (and request url).
  // Tests the request building part of the ConversationAPIClient:
  //  - headers are set correctly when premium credentials are available
  //  - messages are correctly formatted into JSON
  //  - completion response is parsed and passed through to the callbacks
  auto [messages, expected_messages_json] =
      GetMockMessagesAndExpectedMessagesJson();
  std::string expected_system_language = "en_KY";
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale(
      expected_system_language);
  std::string expected_completion_response = "complete text";
  std::string expected_selected_language = "fr";
  std::string expected_capability = "content_agent";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  // Intercept API Request Helper call and verify the request is as expected
  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .WillOnce([&](const std::string& method, const GURL& url,
                    const std::string& body, const std::string& content_type,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback,
                    const base::flat_map<std::string, std::string>& headers,
                    const api_request_helper::APIRequestOptions& options) {
        EXPECT_TRUE(url.is_valid());
        EXPECT_TRUE(url.SchemeIs(url::kHttpsScheme));
        EXPECT_FALSE(body.empty());
        EXPECT_EQ(net::HttpRequestHeaders::kPostMethod, method);

        // Verify headers (non-premium)
        EXPECT_TRUE(headers.contains("x-brave-key"));
        EXPECT_TRUE(headers.contains("digest"));
        EXPECT_TRUE(headers.contains(net::HttpRequestHeaders::kAuthorization));
        EXPECT_FALSE(headers.contains("Cookie"));

        // Verify body
        auto dict = base::test::ParseJsonDict(body);
        EXPECT_TRUE(!dict.empty());

        // Verify body contains the language
        auto [system_language, selected_language] = GetLanguage(dict);
        EXPECT_EQ(system_language, expected_system_language);
        EXPECT_TRUE(selected_language.has_value());
        EXPECT_TRUE(selected_language.value().empty());

        // Verify body contains the brave_capability
        const std::string* capability = dict.FindString("brave_capability");
        EXPECT_TRUE(capability);
        EXPECT_EQ(*capability, expected_capability);

        // Verify body contains the stream
        std::optional<bool> stream = dict.FindBool("stream");
        EXPECT_TRUE(stream.has_value());
        EXPECT_TRUE(*stream);

        // Verify messages content matches expected
        const base::Value::List* messages_list = dict.FindList("messages");
        EXPECT_TRUE(messages_list);
        EXPECT_EQ(*messages_list, expected_messages_json);

        // Simulate streaming chunk
        auto chunk_dict = base::test::ParseJsonDict(R"({
          "choices": [{
            "delta": {"content": "chunk text"},
          }]
        })");
        data_received_callback.Run(
            base::ok(base::Value(std::move(chunk_dict))));

        // Simulate completion
        auto completion_dict = base::test::ParseJsonDict(R"({
          "choices": [{
            "message": {"content": "complete text"},
          }]
        })");
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(
                200, base::Value(std::move(completion_dict)), {}, net::OK,
                GURL()));

        run_loop.Quit();
        return Ticket();
      });

  // Verify callbacks are called
  Sequence seq;
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_completion_event());
        EXPECT_EQ(result.event->get_completion_event()->completion,
                  "chunk text");
      });
  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](EngineConsumer::GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result->event);
        EXPECT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion,
                  expected_completion_response);
      });

  // Begin request
  client_->PerformRequest(
      std::move(messages), "" /* selected_language */,
      std::nullopt, /* oai_tool_definitions */
      std::nullopt, /* preferred_tool_name */
      mojom::ConversationCapability::CONTENT_AGENT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_WithToolUseResponse) {
  // Tests that we interpret tool use reponses. For more variants
  // see tests for `ToolUseEventFromToolCallsResponse`.
  std::vector<OAIMessage> messages =
      GetMockMessagesAndExpectedMessagesJson().first;

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  // Intercept API Request Helper call and verify the request is as expected.
  // Tool use is only supported for streaming requests since the completion
  // callback only supports a single event.
  EXPECT_CALL(*mock_request_helper, RequestSSE)
      .WillOnce([&](const std::string& method, const GURL& url,
                    const std::string& body, const std::string& content_type,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback,
                    const base::flat_map<std::string, std::string>& headers,
                    const api_request_helper::APIRequestOptions& options) {
        {
          // Send response with both content and tool calls
          auto chunk = base::test::ParseJsonDict(R"({
            "choices": [{
              "delta": {
                "content": "This is a test completion",
                "tool_calls": [
                  {
                    "id": "call_123",
                    "type": "function",
                    "function": {
                      "name": "get_weather",
                      "arguments": "{\"location\":\"New York\"}"
                    }
                  },
                  {
                    "id": "call_456",
                    "type": "function",
                    "function": {
                      "name": "search_web",
                      "arguments": "{\"query\":\"Hello, world!\"}"
                    }
                  }
                ]
              }
            }]
          })");
          data_received_callback.Run(base::ok(base::Value(std::move(chunk))));
        }

        // Complete the request
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, {}, {}, net::OK,
                                                      GURL()));
        run_loop.Quit();
        return Ticket();
      });

  Sequence seq;
  EXPECT_CALL(mock_callbacks, OnDataReceived)
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_completion_event());
        EXPECT_EQ(result.event->get_completion_event()->completion,
                  "This is a test completion");
      });

  EXPECT_CALL(mock_callbacks, OnDataReceived)
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        ASSERT_TRUE(result.event->is_tool_use_event());
        EXPECT_MOJOM_EQ(
            result.event->get_tool_use_event(),
            mojom::ToolUseEvent::New("get_weather", "call_123",
                                     "{\"location\":\"New York\"}",
                                     std::nullopt, std::nullopt, nullptr));
      });

  EXPECT_CALL(mock_callbacks, OnDataReceived)
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        ASSERT_TRUE(result.event->is_tool_use_event());
        EXPECT_MOJOM_EQ(
            result.event->get_tool_use_event(),
            mojom::ToolUseEvent::New("search_web", "call_456",
                                     "{\"query\":\"Hello, world!\"}",
                                     std::nullopt, std::nullopt, nullptr));
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](const EngineConsumer::GenerationResult& result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_FALSE(result->event);
        EXPECT_FALSE(result->model_key.has_value());
      });

  // The payload of the request is not important for this test
  client_->PerformRequest(
      std::move(messages), "" /* selected_language */,
      std::nullopt, /* oai_tool_definitions */
      std::nullopt, /* preferred_tool_name */
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
}

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_NonStreaming) {
  std::string expected_completion_response = "complete text";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  auto [messages, expected_messages_json] =
      GetMockMessagesAndExpectedMessagesJson();

  EXPECT_CALL(*mock_request_helper, Request(_, _, _, _, _, _, _, _))
      .WillOnce(
          [&](const std::string& method, const GURL& url,
              const std::string& body, const std::string& content_type,
              ResultCallback result_callback,
              const base::flat_map<std::string, std::string>& headers,
              const api_request_helper::APIRequestOptions& options,
              api_request_helper::APIRequestHelper::ResponseConversionCallback
                  conversion_callback) {
            // Verify Request (not RequestSSE) is called
            EXPECT_EQ(method, net::HttpRequestHeaders::kPostMethod);

            // Verify headers (non-premium, default)
            EXPECT_TRUE(headers.contains("x-brave-key"));
            EXPECT_TRUE(headers.contains("digest"));
            EXPECT_TRUE(
                headers.contains(net::HttpRequestHeaders::kAuthorization));
            EXPECT_FALSE(headers.contains("Cookie"));

            // Verify body has stream: false
            auto dict = base::test::ParseJsonDict(body);
            std::optional<bool> stream = dict.FindBool("stream");
            EXPECT_TRUE(stream.has_value());
            EXPECT_FALSE(*stream);

            // Verify messages content matches expected
            const base::Value::List* messages_list = dict.FindList("messages");
            EXPECT_TRUE(messages_list);
            EXPECT_EQ(*messages_list, expected_messages_json);

            // Simulate non-streaming completion
            auto completion_dict = base::test::ParseJsonDict(R"({
          "choices": [{
            "message": {"content": "complete text"},
          }]
        })");
            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(
                    200, base::Value(std::move(completion_dict)), {}, net::OK,
                    GURL()));

            run_loop.Quit();
            return Ticket();
          });

  // Verify callback is called
  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](EngineConsumer::GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result->event);
        EXPECT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion,
                  expected_completion_response);
      });

  client_->PerformRequest(
      std::move(messages), "en", std::nullopt, std::nullopt,
      mojom::ConversationCapability::CHAT,
      base::NullCallback(),  // No data_received_callback (non-streaming)
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIV2ClientUnitTest,
       PerformRequest_WithModelNameOverride_Streaming) {
  // Tests that the model name override is correctly passed to the API
  auto [messages, expected_messages_json] =
      GetMockMessagesAndExpectedMessagesJson();
  std::string override_model_name = "llama-3-8b-instruct";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  // Intercept API Request Helper call and verify the request is as expected
  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .WillOnce([&](const std::string& method, const GURL& url,
                    const std::string& body, const std::string& content_type,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback,
                    const base::flat_map<std::string, std::string>& headers,
                    const api_request_helper::APIRequestOptions& options) {
        // Verify the model name was overridden in the request
        auto dict = base::test::ParseJsonDict(body);
        const std::string* model = dict.FindString("model");
        EXPECT_TRUE(model);
        EXPECT_EQ(*model, override_model_name);

        // Simulate streaming chunk
        auto chunk_dict = base::test::ParseJsonDict(R"({
          "model": "llama-3-8b-instruct",
          "choices": [{
            "delta": {"content": "This is a test completion"}
          }]
        })");
        data_received_callback.Run(
            base::ok(base::Value(std::move(chunk_dict))));

        // Complete the request
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, {}, {}, net::OK,
                                                      GURL()));
        run_loop.Quit();
        return Ticket();
      });

  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_completion_event());
        EXPECT_EQ(result.event->get_completion_event()->completion,
                  "This is a test completion");
        EXPECT_EQ(result.model_key, "chat-basic");
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](const EngineConsumer::GenerationResult& result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_FALSE(result->event);
        EXPECT_FALSE(result->model_key.has_value());
      });

  // Begin request with model override
  client_->PerformRequest(
      std::move(messages), "" /* selected_language */,
      std::nullopt, /* oai_tool_definitions */
      std::nullopt, /* preferred_tool_name */
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)),
      override_model_name);

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIV2ClientUnitTest,
       PerformRequest_WithModelNameOverride_NonStreaming) {
  // Tests that the non-streaming version (Request) is called with null
  // callback
  auto messages = GetMockMessagesAndExpectedMessagesJson().first;
  std::string override_model_name = "llama-3-8b-instruct";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  // RequestSSE should NOT be called when data_received_callback is null
  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .Times(0);

  // Instead, Request should be called
  EXPECT_CALL(*mock_request_helper, Request(_, _, _, _, _, _, _, _))
      .WillOnce(
          [&](const std::string& method, const GURL& url,
              const std::string& body, const std::string& content_type,
              ResultCallback result_callback,
              const base::flat_map<std::string, std::string>& headers,
              const api_request_helper::APIRequestOptions& options,
              api_request_helper::APIRequestHelper::ResponseConversionCallback
                  response_conversion_callback) {
            // Verify the model name was overridden in the request
            auto dict = base::test::ParseJsonDict(body);
            const std::string* model = dict.FindString("model");
            EXPECT_TRUE(model);
            EXPECT_EQ(*model, override_model_name);

            // Create a response with both completion and model information
            auto response_dict = base::test::ParseJsonDict(R"({
          "model": "llama-3-8b-instruct",
          "choices": [{
            "message": {"content": "This is a test completion"}
          }]
        })");

            // Complete the request
            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(
                    200, base::Value(std::move(response_dict)), {}, net::OK,
                    GURL()));
            run_loop.Quit();
            return Ticket();
          });

  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](const EngineConsumer::GenerationResult& result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->event);
        ASSERT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion,
                  "This is a test completion");
        EXPECT_EQ(result->model_key, "chat-basic");
      });

  // Begin request with model override but NULL data_received_callback
  client_->PerformRequest(std::move(messages), "" /* selected_language */,
                          std::nullopt, /* oai_tool_definitions */
                          std::nullopt, /* preferred_tool_name */
                          mojom::ConversationCapability::CHAT,
                          base::NullCallback(),
                          base::BindOnce(&MockCallbacks::OnCompleted,
                                         base::Unretained(&mock_callbacks)),
                          override_model_name);

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_FailWithEmptyMessages) {
  // Tests handling invalid request parameters (empty messages)
  std::vector<OAIMessage> messages;

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;

  // Intercept API Request Helper call and verify the request is not made
  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .Times(0);

  // Callbacks should be passed through and translated from APIRequestHelper
  // format.
  EXPECT_CALL(mock_callbacks, OnDataReceived).Times(0);
  EXPECT_CALL(
      mock_callbacks,
      OnCompleted(testing::Eq(base::unexpected(mojom::APIError::None))));

  // Begin request with empty messages
  client_->PerformRequest(
      std::move(messages), "" /* selected_language */,
      std::nullopt, /* oai_tool_definitions */
      std::nullopt, /* preferred_tool_name */
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIV2ClientUnitTest,
       PerformRequest_NullEventUponBadResponse) {
  // Tests handling of successful response with invalid/unparseable body
  auto messages = GetMockMessagesAndExpectedMessagesJson().first;

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, Request(_, _, _, _, _, _, _, _))
      .WillOnce(
          [&](const std::string& method, const GURL& url,
              const std::string& body, const std::string& content_type,
              ResultCallback result_callback,
              const base::flat_map<std::string, std::string>& headers,
              const api_request_helper::APIRequestOptions& options,
              api_request_helper::APIRequestHelper::ResponseConversionCallback
                  conversion_callback) {
            // Simulate successful response with invalid/unparseable body
            auto invalid_response = base::test::ParseJsonDict(R"({
          "invalid_field": "no choices array"
        })");
            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(
                    200, base::Value(std::move(invalid_response)), {}, net::OK,
                    GURL()));

            run_loop.Quit();
            return Ticket();
          });

  // When parsing fails, should return null event
  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](const EngineConsumer::GenerationResult& result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(),
                  EngineConsumer::GenerationResultData(nullptr, std::nullopt));
      });

  client_->PerformRequest(std::move(messages), "" /* selected_language */,
                          std::nullopt, /* oai_tool_definitions */
                          std::nullopt, /* preferred_tool_name */
                          mojom::ConversationCapability::CHAT,
                          base::NullCallback(),
                          base::BindOnce(&MockCallbacks::OnCompleted,
                                         base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_ServerErrorResponse) {
  // Tests handling of server error response (rate limit)
  auto messages = GetMockMessagesAndExpectedMessagesJson().first;

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, Request(_, _, _, _, _, _, _, _))
      .WillOnce(
          [&](const std::string& method, const GURL& url,
              const std::string& body, const std::string& content_type,
              ResultCallback result_callback,
              const base::flat_map<std::string, std::string>& headers,
              const api_request_helper::APIRequestOptions& options,
              api_request_helper::APIRequestHelper::ResponseConversionCallback
                  conversion_callback) {
            // Simulate rate limit error (429)
            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(
                    net::HTTP_TOO_MANY_REQUESTS, base::Value(), {}, net::OK,
                    GURL()));

            run_loop.Quit();
            return Ticket();
          });

  // Should return RateLimitReached error
  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](const EngineConsumer::GenerationResult& result) {
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), mojom::APIError::RateLimitReached);
      });

  client_->PerformRequest(std::move(messages), "" /* selected_language */,
                          std::nullopt, /* oai_tool_definitions */
                          std::nullopt, /* preferred_tool_name */
                          mojom::ConversationCapability::CHAT,
                          base::NullCallback(),
                          base::BindOnce(&MockCallbacks::OnCompleted,
                                         base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

}  // namespace ai_chat
