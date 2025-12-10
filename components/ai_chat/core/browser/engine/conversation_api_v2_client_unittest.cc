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
#include "brave/components/ai_chat/core/browser/engine/extended_content_block.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
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

using ::testing::_;
using ::testing::Sequence;
using DataReceivedCallback =
    api_request_helper::APIRequestHelper::DataReceivedCallback;
using ResultCallback = api_request_helper::APIRequestHelper::ResultCallback;
using Ticket = api_request_helper::APIRequestHelper::Ticket;
using api_request_helper::MockAPIRequestHelper;

namespace ai_chat {

namespace {

struct ContentBlockTestParam {
  std::string name;
  base::RepeatingCallback<ExtendedContentBlock()> get_test_content_block;
  std::string expected_type;
};

std::pair<std::vector<OAIMessage>, base::Value::Dict>
GetMockMessagesAndExpectedContent() {
  std::vector<OAIMessage> messages;

  // Create a simple user message with text content
  OAIMessage message;
  message.role = "user";
  ExtendedContentBlock content_block;
  content_block.type = ExtendedContentBlockType::kText;
  TextContent text_content;
  text_content.text = "test message";
  content_block.data = std::move(text_content);
  message.content.push_back(std::move(content_block));
  messages.push_back(std::move(message));

  // Create expected messages dict for verification
  base::Value::Dict expected_content;
  expected_content.Set("role", "user");
  base::Value::List content_list;
  base::Value::Dict content_dict;
  content_dict.Set("type", "text");
  content_dict.Set("text", "test message");
  content_list.Append(std::move(content_dict));
  expected_content.Set("content", std::move(content_list));

  return std::make_pair(std::move(messages), std::move(expected_content));
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

  ExtendedContentBlock block = params.get_test_content_block.Run();
  std::vector<OAIMessage> messages;
  OAIMessage message;
  message.role = "user";
  message.content.emplace_back(std::move(block));
  messages.push_back(std::move(message));

  base::Value::List serialized =
      ConversationAPIV2Client::SerializeOAIMessages(std::move(messages));

  ASSERT_EQ(serialized.size(), 1u);
  const base::Value::Dict* message_dict = serialized[0].GetIfDict();
  ASSERT_TRUE(message_dict);

  const std::string* role = message_dict->FindString("role");
  ASSERT_TRUE(role);
  EXPECT_EQ(*role, "user");

  const base::Value::List* content_list = message_dict->FindList("content");
  ASSERT_TRUE(content_list);
  ASSERT_EQ(content_list->size(), 1u);

  const base::Value::Dict* content_dict = (*content_list)[0].GetIfDict();
  ASSERT_TRUE(content_dict);

  // Build expected content based on block type
  base::Value::Dict expected_content;
  expected_content.Set("type", params.expected_type);

  ExtendedContentBlock original_block = params.get_test_content_block.Run();
  if (original_block.type == ExtendedContentBlockType::kImage) {
    const ImageContent* img = std::get_if<ImageContent>(&original_block.data);
    ASSERT_TRUE(img);
    base::Value::Dict image_url_dict;
    image_url_dict.Set("url", img->image_url.url);
    if (img->image_url.detail) {
      image_url_dict.Set("detail", *img->image_url.detail);
    }
    expected_content.Set("image_url", std::move(image_url_dict));
  } else if (original_block.type == ExtendedContentBlockType::kChangeTone) {
    const ChangeToneContent* tone =
        std::get_if<ChangeToneContent>(&original_block.data);
    ASSERT_TRUE(tone);
    expected_content.Set("text", "");
    expected_content.Set("tone", tone->tone);
  } else {
    // All other types have TextContent data
    const TextContent* text = std::get_if<TextContent>(&original_block.data);
    ASSERT_TRUE(text);
    expected_content.Set("text", text->text);
  }

  EXPECT_EQ(*content_dict, expected_content);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    ConversationAPIV2ClientUnitTest_ContentBlocks,
    testing::Values(
        ContentBlockTestParam{"Text", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kText,
                                    TextContent{"test content"});
                              }),
                              "text"},
        ContentBlockTestParam{
            "Image", base::BindRepeating([]() {
              ImageContent img;
              img.image_url.url = "data:image/png;base64,abc123";
              img.image_url.detail = std::optional<std::string>("high");
              return ExtendedContentBlock(ExtendedContentBlockType::kImage,
                                          std::move(img));
            }),
            "image_url"},
        ContentBlockTestParam{"PageExcerpt", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kPageExcerpt,
                                    TextContent{"test content"});
                              }),
                              "brave-page-excerpt"},
        ContentBlockTestParam{"PageText", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kPageText,
                                    TextContent{"test page content"});
                              }),
                              "brave-page-text"},
        ContentBlockTestParam{"VideoTranscript", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kVideoTranscript,
                                    TextContent{"test video transcript"});
                              }),
                              "brave-video-transcript"},
        ContentBlockTestParam{"RequestSummary", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kRequestSummary,
                                    TextContent{""});
                              }),
                              "brave-request-summary"},
        ContentBlockTestParam{"RequestQuestions", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kRequestQuestions,
                                    TextContent{""});
                              }),
                              "brave-request-questions"},
        ContentBlockTestParam{"Paraphrase", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kParaphrase,
                                    TextContent{"test content"});
                              }),
                              "brave-request-paraphrase"},
        ContentBlockTestParam{"Improve", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kImprove,
                                    TextContent{"test content"});
                              }),
                              "brave-request-improve-excerpt-language"},
        ContentBlockTestParam{"Shorten", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kShorten,
                                    TextContent{"test content"});
                              }),
                              "brave-request-shorten"},
        ContentBlockTestParam{"Expand", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kExpand,
                                    TextContent{"test content"});
                              }),
                              "brave-request-expansion"},
        ContentBlockTestParam{"ChangeTone", base::BindRepeating([]() {
                                return ExtendedContentBlock(
                                    ExtendedContentBlockType::kChangeTone,
                                    ChangeToneContent{"professional"});
                              }),
                              "brave-request-change-tone"}),
    [](const testing::TestParamInfo<ContentBlockTestParam>& info) {
      return info.param.name;
    });

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_PremiumHeaders) {
  // Tests the request building part of the ConversationAPIClient:
  //  - headers are set correctly when premium credentials are available
  //  - messages are correctly formatted into JSON
  //  - completion response is parsed and passed through to the callbacks
  std::string expected_credential = "test-premium-credential";
  auto [messages, expected_content] = GetMockMessagesAndExpectedContent();
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
        EXPECT_EQ(messages_list->size(), 1u);
        const base::Value::Dict* message_dict = (*messages_list)[0].GetIfDict();
        EXPECT_TRUE(message_dict);
        EXPECT_EQ(*message_dict, expected_content);

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
  auto [messages, expected_content] = GetMockMessagesAndExpectedContent();
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
        EXPECT_EQ(messages_list->size(), 1u);
        const base::Value::Dict* message_dict = (*messages_list)[0].GetIfDict();
        EXPECT_TRUE(message_dict);
        EXPECT_EQ(*message_dict, expected_content);

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

TEST_F(ConversationAPIV2ClientUnitTest, PerformRequest_NonStreaming) {
  std::string expected_completion_response = "complete text";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  auto [messages, expected_content] = GetMockMessagesAndExpectedContent();

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
            EXPECT_EQ(messages_list->size(), 1u);
            const base::Value::Dict* message_dict =
                (*messages_list)[0].GetIfDict();
            EXPECT_TRUE(message_dict);
            EXPECT_EQ(*message_dict, expected_content);

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
  auto [messages, expected_content] = GetMockMessagesAndExpectedContent();
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
  auto messages = GetMockMessagesAndExpectedContent().first;
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
  auto messages = GetMockMessagesAndExpectedContent().first;

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
  auto messages = GetMockMessagesAndExpectedContent().first;

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
