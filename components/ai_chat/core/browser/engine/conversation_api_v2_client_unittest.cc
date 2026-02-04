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
#include "brave/components/ai_chat/core/browser/constants.h"
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

std::pair<std::vector<OAIMessage>, base::ListValue>
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
        nullptr, false));
    message.tool_calls.push_back(
        mojom::ToolUseEvent::New("get_screenshot", "456", "{\"type\":\"tab\"}",
                                 std::nullopt, nullptr, false));
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
  // The system language is the browser language
  std::string GetSystemLanguage(const base::DictValue& body) {
    const std::string* system_language = body.FindString("system_language");
    // The system language should always be present
    EXPECT_TRUE(system_language != nullptr);
    return *system_language;
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

  base::ListValue serialized =
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
            })"},
        ContentBlockTestParam{
            "WebSources", base::BindRepeating([]() {
              std::vector<mojom::WebSourcePtr> sources;
              sources.push_back(mojom::WebSource::New(
                  "Example Title", GURL("https://example.com/page"),
                  GURL("https://example.com/favicon.ico")));
              return mojom::ContentBlock::NewWebSourcesContentBlock(
                  mojom::WebSourcesContentBlock::New(std::move(sources),
                                                     "test query"));
            }),
            R"({
              "type": "brave-chat.webSources",
              "sources": [{
                "title": "Example Title",
                "url": "https://example.com/page",
                "favicon": "https://example.com/favicon.ico"
              }],
              "query": "test query"
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

        // Verify body contains the system language
        auto system_language = GetSystemLanguage(body_dict);
        EXPECT_EQ(system_language, expected_system_language);

        // Currently server only expects we pass content_agent capability,
        // so it won't be passed for CHAT.
        EXPECT_FALSE(body_dict.FindString("brave_capability"));

        // Verify body contains the stream
        std::optional<bool> stream = body_dict.FindBool("stream");
        EXPECT_TRUE(stream);
        EXPECT_TRUE(*stream);

        // Verify messages content matches expected
        const base::ListValue* messages_list = body_dict.FindList("messages");
        EXPECT_TRUE(messages_list);
        EXPECT_EQ(*messages_list, expected_messages_json);

        // Simulate streaming chunk
        auto chunk_dict = base::test::ParseJsonDict(R"({
          "object": "chat.completion.chunk",
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
        EXPECT_FALSE(result->is_near_verified.has_value());
      });

  // Begin request
  client_->PerformRequest(
      std::move(messages), std::nullopt,
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

        // Verify body contains the system language
        auto system_language = GetSystemLanguage(dict);
        EXPECT_EQ(system_language, expected_system_language);

        // Verify body contains the brave_capability
        const std::string* capability = dict.FindString("brave_capability");
        EXPECT_TRUE(capability);
        EXPECT_EQ(*capability, expected_capability);

        // Verify body contains the stream
        std::optional<bool> stream = dict.FindBool("stream");
        EXPECT_TRUE(stream.has_value());
        EXPECT_TRUE(*stream);

        // Verify messages content matches expected
        const base::ListValue* messages_list = dict.FindList("messages");
        EXPECT_TRUE(messages_list);
        EXPECT_EQ(*messages_list, expected_messages_json);

        // Simulate streaming chunk
        auto chunk_dict = base::test::ParseJsonDict(R"({
          "object": "chat.completion.chunk",
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
      std::move(messages), std::nullopt, /* oai_tool_definitions */
      std::nullopt,                      /* preferred_tool_name */
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
            "object": "chat.completion.chunk",
            "model": "llama-3-8b-instruct",
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
        EXPECT_EQ(result.model_key, "chat-basic");
      });

  EXPECT_CALL(mock_callbacks, OnDataReceived)
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        ASSERT_TRUE(result.event->is_tool_use_event());
        EXPECT_MOJOM_EQ(result.event->get_tool_use_event(),
                        mojom::ToolUseEvent::New("get_weather", "call_123",
                                                 "{\"location\":\"New York\"}",
                                                 std::nullopt, nullptr, false));
        EXPECT_EQ(result.model_key, "chat-basic");
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
                                     std::nullopt, nullptr, false));
        EXPECT_EQ(result.model_key, "chat-basic");
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](const EngineConsumer::GenerationResult& result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_FALSE(result->event);
        EXPECT_FALSE(result->model_key.has_value());
      });

  // The payload of the request is not important for this test
  client_->PerformRequest(
      std::move(messages), std::nullopt, /* oai_tool_definitions */
      std::nullopt,                      /* preferred_tool_name */
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
}

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_PermissionChallenge) {
  // Tests that we correctly parse alignment_check from each tool call:
  // - Populate PermissionChallenge when allowed is false
  // - Don't populate PermissionChallenge when allowed is true
  // - Handle unknown alignment_check schema (missing allowed property) by
  //   ignoring the alignment_check
  // - Ignore missing reasoning property and still provide PermissionChallenge
  std::vector<OAIMessage> messages =
      GetMockMessagesAndExpectedMessagesJson().first;
  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::NiceMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, RequestSSE)
      .WillOnce(testing::WithArgs<4, 5>(
          [&](DataReceivedCallback data_received_callback,
              ResultCallback result_callback) {
            auto chunk = base::test::ParseJsonDict(R"({
              "object": "chat.completion.chunk",
              "choices": [{
                "delta": {
                  "content": "This is a test completion",
                  "tool_calls": [
                    {
                      "id": "call_123",
                      "type": "function",
                      "function": {
                        "name": "search_web",
                        "arguments": "{\"query\":\"Hello, world!\"}"
                      },
                      "alignment_check": {
                        "allowed": false,
                        "reasoning": "Server determined this tool use is off"
                      }
                    },
                    {
                      "id": "call_456",
                      "type": "function",
                      "function": {
                        "name": "get_weather",
                        "arguments": "{\"location\":\"New York\"}"
                      }
                    },
                    {
                      "id": "call_789",
                      "type": "function",
                      "function": {
                        "name": "read_file",
                        "arguments": "{\"path\":\"/etc/passwd\"}"
                      },
                      "alignment_check": {
                        "allowed": false,
                        "reasoning": "This tool is also off-topic"
                      }
                    },
                    {
                      "id": "call_101",
                      "type": "function",
                      "function": {
                        "name": "allowed_tool",
                        "arguments": "{\"arg\":\"value\"}"
                      },
                      "alignment_check": {
                        "allowed": true,
                        "reasoning": "This is allowed"
                      }
                    },
                    {
                      "id": "call_202",
                      "type": "function",
                      "function": {
                        "name": "missing_allowed_field",
                        "arguments": "{}"
                      },
                      "alignment_check": {
                        "reasoning": "Format unknown"
                      }
                    },
                    {
                      "id": "call_303",
                      "type": "function",
                      "function": {
                        "name": "missing_reasoning",
                        "arguments": "{}"
                      },
                      "alignment_check": {
                        "allowed": false
                      }
                    }
                  ]
                }
              }]
            })");
            data_received_callback.Run(base::ok(base::Value(std::move(chunk))));

            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(200, {}, {}, net::OK,
                                                          GURL()));
            run_loop.QuitWhenIdle();
            return Ticket();
          }));

  // This test is focused on the correctness of the ToolUseEvent,
  // we can leave verifying other events are also sent in another test.
  EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(testing::AnyNumber());

  auto expected_tool_use_event_1 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "search_web", "call_123", "{\"query\":\"Hello, world!\"}",
          std::nullopt,
          mojom::PermissionChallenge::New(
              "Server determined this tool use is off", std::nullopt),
          false));
  {
    SCOPED_TRACE(
        "Expected search_web (call_123) to have PermissionChallenge with "
        "reasoning when alignment_check.allowed=false");
    EXPECT_CALL(mock_callbacks,
                OnDataReceived(testing::Field(
                    "event", &EngineConsumer::GenerationResultData::event,
                    MojomEq(expected_tool_use_event_1.get()))))
        .Times(1);
  }

  auto expected_tool_use_event_2 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_456", "{\"location\":\"New York\"}",
          std::nullopt, nullptr, false));
  {
    SCOPED_TRACE(
        "Expected get_weather (call_456) to have no PermissionChallenge "
        "when alignment_check is not present");
    EXPECT_CALL(mock_callbacks,
                OnDataReceived(testing::Field(
                    "event", &EngineConsumer::GenerationResultData::event,
                    MojomEq(expected_tool_use_event_2.get()))))
        .Times(1);
  }

  auto expected_tool_use_event_3 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "read_file", "call_789", "{\"path\":\"/etc/passwd\"}", std::nullopt,
          mojom::PermissionChallenge::New("This tool is also off-topic",
                                          std::nullopt),
          false));
  {
    SCOPED_TRACE(
        "Expected read_file (call_789) to have PermissionChallenge with "
        "reasoning when alignment_check.allowed=false");
    EXPECT_CALL(mock_callbacks,
                OnDataReceived(testing::Field(
                    "event", &EngineConsumer::GenerationResultData::event,
                    MojomEq(expected_tool_use_event_3.get()))))
        .Times(1);
  }

  auto expected_tool_use_event_4 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "allowed_tool", "call_101", "{\"arg\":\"value\"}", std::nullopt,
          nullptr, false));
  {
    SCOPED_TRACE(
        "Expected allowed_tool (call_101) to have no PermissionChallenge "
        "when alignment_check.allowed=true");
    EXPECT_CALL(mock_callbacks,
                OnDataReceived(testing::Field(
                    "event", &EngineConsumer::GenerationResultData::event,
                    MojomEq(expected_tool_use_event_4.get()))))
        .Times(1);
  }

  auto expected_tool_use_event_5 =
      mojom::ConversationEntryEvent::NewToolUseEvent(
          mojom::ToolUseEvent::New("missing_allowed_field", "call_202", "{}",
                                   std::nullopt, nullptr, false));
  {
    SCOPED_TRACE(
        "Expected missing_allowed_field (call_202) to have no "
        "PermissionChallenge when alignment_check.allowed field is missing");
    EXPECT_CALL(mock_callbacks,
                OnDataReceived(testing::Field(
                    "event", &EngineConsumer::GenerationResultData::event,
                    MojomEq(expected_tool_use_event_5.get()))))
        .Times(1);
  }

  auto expected_tool_use_event_6 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "missing_reasoning", "call_303", "{}", std::nullopt,
          mojom::PermissionChallenge::New(std::nullopt, std::nullopt), false));
  {
    SCOPED_TRACE(
        "Expected missing_reasoning (call_303) to have PermissionChallenge "
        "without reasoning when alignment_check.allowed=false but reasoning "
        "is missing");
    EXPECT_CALL(mock_callbacks,
                OnDataReceived(testing::Field(
                    "event", &EngineConsumer::GenerationResultData::event,
                    MojomEq(expected_tool_use_event_6.get()))))
        .Times(1);
  }

  client_->PerformRequest(
      std::move(messages), std::nullopt /* oai_tool_definitions */,
      std::nullopt /* preferred_tool_name */,
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
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
            const base::ListValue* messages_list = dict.FindList("messages");
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
      std::move(messages), std::nullopt, std::nullopt,
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
          "object": "chat.completion.chunk",
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
      std::move(messages), std::nullopt, /* oai_tool_definitions */
      std::nullopt,                      /* preferred_tool_name */
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
  client_->PerformRequest(
      std::move(messages), std::nullopt, /* oai_tool_definitions */
      std::nullopt,                      /* preferred_tool_name */
      mojom::ConversationCapability::CHAT, base::NullCallback(),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)),
      override_model_name);

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_NEARVerification) {
  std::string expected_completion_response = "Verified response";
  auto [messages, expected_messages_json] =
      GetMockMessagesAndExpectedMessagesJson();

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .WillOnce([&](const std::string& method, const GURL& url,
                    const std::string& body, const std::string& content_type,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback,
                    const base::flat_map<std::string, std::string>& headers,
                    const api_request_helper::APIRequestOptions& options) {
        // Simulate completion
        auto completion_dict = base::test::ParseJsonDict(R"({
          "object": "chat.completion.chunk",
          "model": "llama-3-8b-instruct",
          "choices": [{
            "delta": {"content": "Verified response"}
          }]
        })");
        data_received_callback.Run(
            base::ok(base::Value(std::move(completion_dict))));

        base::flat_map<std::string, std::string> response_headers;
        response_headers[kBraveNearVerifiedHeader] = "true";
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, {}, response_headers,
                                                      net::OK, GURL()));
        run_loop.Quit();
        return Ticket();
      });

  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_completion_event());
        EXPECT_EQ(result.event->get_completion_event()->completion,
                  expected_completion_response);
        EXPECT_FALSE(result.is_near_verified);
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([](EngineConsumer::GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result->is_near_verified.has_value());
        EXPECT_TRUE(result->is_near_verified.value());
      });

  client_->PerformRequest(
      std::move(messages), std::nullopt, /* oai_tool_definitions */
      std::nullopt,                      /* preferred_tool_name */
      mojom::ConversationCapability::CONTENT_AGENT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
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
      std::move(messages), std::nullopt, /* oai_tool_definitions */
      std::nullopt,                      /* preferred_tool_name */
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

  client_->PerformRequest(
      std::move(messages), std::nullopt, /* oai_tool_definitions */
      std::nullopt,                      /* preferred_tool_name */
      mojom::ConversationCapability::CHAT, base::NullCallback(),
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

  client_->PerformRequest(
      std::move(messages), std::nullopt, /* oai_tool_definitions */
      std::nullopt,                      /* preferred_tool_name */
      mojom::ConversationCapability::CHAT, base::NullCallback(),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIV2ClientUnitTest, OnQueryDataReceived_ContentReceipt) {
  // Test content receipt event parsing in OnQueryDataReceived
  testing::StrictMock<MockCallbacks> mock_callbacks;

  // Case 1: Normal case with both total_tokens and trimmed_tokens present
  {
    SCOPED_TRACE("Both total_tokens and trimmed_tokens present");
    auto content_receipt = base::test::ParseJsonDict(R"({
      "object": "brave-chat.contentReceipt",
      "model": "llama-3-8b-instruct",
      "total_tokens": 1234567890,
      "trimmed_tokens": 987654321
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_content_receipt_event());
          EXPECT_EQ(result.event->get_content_receipt_event()->total_tokens,
                    1234567890u);
          EXPECT_EQ(result.event->get_content_receipt_event()->trimmed_tokens,
                    987654321u);
          EXPECT_EQ(result.model_key, "chat-basic");
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(content_receipt))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 2: Both values missing (should default to 0)
  {
    SCOPED_TRACE("Both total_tokens and trimmed_tokens missing");
    auto content_receipt = base::test::ParseJsonDict(R"({
      "object": "brave-chat.contentReceipt",
      "model": "llama-3-8b-instruct"
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_content_receipt_event());
          EXPECT_EQ(result.event->get_content_receipt_event()->total_tokens,
                    0u);
          EXPECT_EQ(result.event->get_content_receipt_event()->trimmed_tokens,
                    0u);
          EXPECT_EQ(result.model_key, "chat-basic");
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(content_receipt))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 3: Only total_tokens present
  {
    SCOPED_TRACE("Only total_tokens present");
    auto content_receipt = base::test::ParseJsonDict(R"({
      "object": "brave-chat.contentReceipt",
      "model": "llama-3-8b-instruct",
      "total_tokens": 5000
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_content_receipt_event());
          EXPECT_EQ(result.event->get_content_receipt_event()->total_tokens,
                    5000u);
          EXPECT_EQ(result.event->get_content_receipt_event()->trimmed_tokens,
                    0u);
          EXPECT_EQ(result.model_key, "chat-basic");
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(content_receipt))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 4: Only trimmed_tokens present
  {
    SCOPED_TRACE("Only trimmed_tokens present");
    auto content_receipt = base::test::ParseJsonDict(R"({
      "object": "brave-chat.contentReceipt",
      "model": "llama-3-8b-instruct",
      "trimmed_tokens": 3000
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_content_receipt_event());
          EXPECT_EQ(result.event->get_content_receipt_event()->total_tokens,
                    0u);
          EXPECT_EQ(result.event->get_content_receipt_event()->trimmed_tokens,
                    3000u);
          EXPECT_EQ(result.model_key, "chat-basic");
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(content_receipt))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 5: Negative values (should default to 0)
  {
    SCOPED_TRACE("Negative values default to 0");
    auto content_receipt = base::test::ParseJsonDict(R"({
      "object": "brave-chat.contentReceipt",
      "model": "llama-3-8b-instruct",
      "total_tokens": -100,
      "trimmed_tokens": -50
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_content_receipt_event());
          EXPECT_EQ(result.event->get_content_receipt_event()->total_tokens,
                    0u);
          EXPECT_EQ(result.event->get_content_receipt_event()->trimmed_tokens,
                    0u);
          EXPECT_EQ(result.model_key, "chat-basic");
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(content_receipt))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 6: Mixed values (one positive, one negative)
  {
    SCOPED_TRACE("Mixed values - positive total, negative trimmed");
    auto content_receipt = base::test::ParseJsonDict(R"({
      "object": "brave-chat.contentReceipt",
      "model": "llama-3-8b-instruct",
      "total_tokens": 8000,
      "trimmed_tokens": -200
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_content_receipt_event());
          EXPECT_EQ(result.event->get_content_receipt_event()->total_tokens,
                    8000u);
          EXPECT_EQ(result.event->get_content_receipt_event()->trimmed_tokens,
                    0u);
          EXPECT_EQ(result.model_key, "chat-basic");
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(content_receipt))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }
}

TEST_F(ConversationAPIV2ClientUnitTest, OnQueryDataReceived_ToolStart) {
  // Test toolStart event parsing for server-side search tools
  testing::StrictMock<MockCallbacks> mock_callbacks;

  // Case 1: brave_web_search tool should emit SearchStatusEvent
  {
    SCOPED_TRACE("brave_web_search should emit SearchStatusEvent");
    auto tool_start = base::test::ParseJsonDict(R"({
      "object": "brave-chat.toolStart",
      "tool_name": "brave_web_search"
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_search_status_event());
          EXPECT_TRUE(result.event->get_search_status_event()->is_searching);
          EXPECT_FALSE(result.model_key.has_value());
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(tool_start))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 2: brave_news_search tool should emit SearchStatusEvent
  {
    SCOPED_TRACE("brave_news_search should emit SearchStatusEvent");
    auto tool_start = base::test::ParseJsonDict(R"({
      "object": "brave-chat.toolStart",
      "tool_name": "brave_news_search"
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_search_status_event());
          EXPECT_TRUE(result.event->get_search_status_event()->is_searching);
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(tool_start))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 3: Non-search tool should NOT emit SearchStatusEvent
  {
    SCOPED_TRACE("page_summary should not emit SearchStatusEvent");
    auto tool_start = base::test::ParseJsonDict(R"({
      "object": "brave-chat.toolStart",
      "tool_name": "page_summary"
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(0);

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(tool_start))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 4: Empty tool_name should NOT emit SearchStatusEvent
  {
    SCOPED_TRACE("Empty tool_name should not emit SearchStatusEvent");
    auto tool_start = base::test::ParseJsonDict(R"({
      "object": "brave-chat.toolStart",
      "tool_name": ""
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(0);

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(tool_start))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 5: Missing tool_name should NOT emit SearchStatusEvent
  {
    SCOPED_TRACE("Missing tool_name should not emit SearchStatusEvent");
    auto tool_start = base::test::ParseJsonDict(R"({
      "object": "brave-chat.toolStart"
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(0);

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(tool_start))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }
}

TEST_F(ConversationAPIV2ClientUnitTest, OnQueryDataReceived_ToolCallRequest) {
  // Test tool call request parsing (function dict present)
  testing::StrictMock<MockCallbacks> mock_callbacks;

  auto chunk = base::test::ParseJsonDict(R"({
    "object": "chat.completion.chunk",
    "model": "llama-3-8b-instruct",
    "choices": [{
      "delta": {
        "tool_calls": [
          {
            "id": "call_123",
            "type": "function",
            "function": {
              "name": "brave_web_search",
              "arguments": "{\"query\":\"weather today\"}"
            }
          }
        ]
      }
    }]
  })");

  auto expected_event =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "brave_web_search", "call_123", "{\"query\":\"weather today\"}",
          std::nullopt, nullptr, false));
  EXPECT_CALL(mock_callbacks,
              OnDataReceived(testing::Field(
                  "event", &EngineConsumer::GenerationResultData::event,
                  MojomEq(expected_event.get()))))
      .Times(1);

  client_->OnQueryDataReceived(
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::ok(base::Value(std::move(chunk))));

  testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
}

TEST_F(ConversationAPIV2ClientUnitTest, OnQueryDataReceived_ToolCallResult) {
  // Test tool call result parsing (output_content present)
  testing::StrictMock<MockCallbacks> mock_callbacks;

  auto chunk = base::test::ParseJsonDict(R"({
    "object": "chat.completion.chunk",
    "model": "llama-3-8b-instruct",
    "choices": [{
      "delta": {
        "tool_calls": [
          {
            "id": "call_123",
            "output_content": [
              {
                "type": "brave-chat.webSources",
                "sources": [
                  {
                    "title": "Weather.com",
                    "url": "https://weather.com",
                    "favicon": "https://imgs.search.brave.com/weather.ico"
                  }
                ],
                "query": "weather today"
              }
            ]
          }
        ]
      }
    }]
  })");

  std::vector<mojom::WebSourcePtr> sources;
  sources.push_back(
      mojom::WebSource::New("Weather.com", GURL("https://weather.com"),
                            GURL("https://imgs.search.brave.com/weather.ico")));
  std::vector<mojom::ContentBlockPtr> output;
  output.push_back(mojom::ContentBlock::NewWebSourcesContentBlock(
      mojom::WebSourcesContentBlock::New(std::move(sources), "weather today")));
  auto expected_event =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "", "call_123", std::string(), std::move(output), nullptr, true));
  EXPECT_CALL(mock_callbacks,
              OnDataReceived(testing::Field(
                  "event", &EngineConsumer::GenerationResultData::event,
                  MojomEq(expected_event.get()))))
      .Times(1);

  client_->OnQueryDataReceived(
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::ok(base::Value(std::move(chunk))));

  testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
}

TEST_F(ConversationAPIV2ClientUnitTest, OnQueryDataReceived_CompletionChunk) {
  // Test streaming completion chunk parsing in OnQueryDataReceived
  testing::StrictMock<MockCallbacks> mock_callbacks;

  // Case 1: Normal chat.completion.chunk with delta content
  {
    SCOPED_TRACE("Normal chunk with delta content");
    auto chunk = base::test::ParseJsonDict(R"({
      "id": "chatcmpl-123",
      "object": "chat.completion.chunk",
      "created": 1677652288,
      "model": "llama-3-8b-instruct",
      "choices": [{
        "index": 0,
        "delta": {
          "role": "assistant",
          "content": "This is a chunk."
        },
        "finish_reason": null
      }]
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_completion_event());
          EXPECT_EQ(result.event->get_completion_event()->completion,
                    "This is a chunk.");
          EXPECT_EQ(result.model_key, "chat-basic");
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(chunk))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 2: Chunk with empty content (should not call callback)
  {
    SCOPED_TRACE("Chunk with empty content - no callback");
    auto chunk = base::test::ParseJsonDict(R"({
      "id": "chatcmpl-456",
      "object": "chat.completion.chunk",
      "created": 1677652288,
      "model": "llama-3-8b-instruct",
      "choices": [{
        "index": 0,
        "delta": {
          "content": ""
        },
        "finish_reason": null
      }]
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(0);

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(chunk))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 3: Chunk without model (model_key should be nullopt)
  {
    SCOPED_TRACE("Chunk without model field");
    auto chunk = base::test::ParseJsonDict(R"({
      "id": "chatcmpl-789",
      "object": "chat.completion.chunk",
      "created": 1677652288,
      "choices": [{
        "index": 0,
        "delta": {
          "content": "Chunk without model."
        },
        "finish_reason": null
      }]
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_completion_event());
          EXPECT_EQ(result.event->get_completion_event()->completion,
                    "Chunk without model.");
          EXPECT_FALSE(result.model_key.has_value());
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(chunk))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }

  // Case 4: Chunk with unknown model name (model_key should be nullopt)
  {
    SCOPED_TRACE("Chunk with unknown model name");
    auto chunk = base::test::ParseJsonDict(R"({
      "id": "chatcmpl-999",
      "object": "chat.completion.chunk",
      "created": 1677652288,
      "model": "unknown-model-name",
      "choices": [{
        "index": 0,
        "delta": {
          "content": "Chunk with unknown model."
        },
        "finish_reason": null
      }]
    })");

    EXPECT_CALL(mock_callbacks, OnDataReceived(_))
        .WillOnce([&](EngineConsumer::GenerationResultData result) {
          ASSERT_TRUE(result.event);
          ASSERT_TRUE(result.event->is_completion_event());
          EXPECT_EQ(result.event->get_completion_event()->completion,
                    "Chunk with unknown model.");
          EXPECT_FALSE(result.model_key.has_value());
        });

    client_->OnQueryDataReceived(
        base::BindRepeating(&MockCallbacks::OnDataReceived,
                            base::Unretained(&mock_callbacks)),
        base::ok(base::Value(std::move(chunk))));

    testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
  }
}

}  // namespace ai_chat
