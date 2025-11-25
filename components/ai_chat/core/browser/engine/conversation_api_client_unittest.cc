/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"

#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/numerics/clamped_math.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/api_request_helper/mock_api_request_helper.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

class PrefService;
namespace mojo {
template <typename Interface>
class PendingRemote;
}  // namespace mojo
namespace skus {
namespace mojom {
class SkusService;
}  // namespace mojom
}  // namespace skus

using ConversationHistory = std::vector<ai_chat::mojom::ConversationTurn>;
using ::testing::_;
using ::testing::Sequence;
using DataReceivedCallback =
    api_request_helper::APIRequestHelper::DataReceivedCallback;
using ResultCallback = api_request_helper::APIRequestHelper::ResultCallback;
using Ticket = api_request_helper::APIRequestHelper::Ticket;
using ResponseConversionCallback =
    api_request_helper::APIRequestHelper::ResponseConversionCallback;
using api_request_helper::MockAPIRequestHelper;

namespace ai_chat {

using ConversationEventRole = ConversationAPIClient::ConversationEventRole;
using ConversationEventType = ConversationAPIClient::ConversationEventType;

namespace {

// Use an initializer list to construct a vector of mojom::ToolUseEventPtr
std::vector<mojom::ToolUseEventPtr> MakeToolUseEvents(
    std::initializer_list<mojom::ToolUseEventPtr> tool_calls) {
  std::vector<mojom::ToolUseEventPtr> tool_use_events;
  for (const auto& tool_call : tool_calls) {
    tool_use_events.emplace_back(tool_call->Clone());
  }
  return tool_use_events;
}

ConversationAPIClient::Content MakeContentBlocks(
    std::initializer_list<mojom::ContentBlockPtr> blocks) {
  std::vector<mojom::ContentBlockPtr> content_blocks;
  for (const auto& block : blocks) {
    content_blocks.emplace_back(block->Clone());
  }
  return content_blocks;
}

std::pair<std::vector<ConversationAPIClient::ConversationEvent>, std::string>
GetMockEventsAndExpectedEventsBody() {
  std::pair<std::vector<ConversationAPIClient::ConversationEvent>, std::string>
      mock_events_and_expected_events_body;

  std::vector<ConversationAPIClient::ConversationEvent> events;
  events.emplace_back(
      ConversationEventRole::kUser, ConversationEventType::kUserMemory,
      std::vector<std::string>{}, "",
      base::Value::Dict()
          .Set("name", "Jane")
          .Set("memories",
               base::Value::List().Append("memory1").Append("memory2")));
  events.emplace_back(
      ConversationEventRole::kUser, ConversationEventType::kPageText,
      std::vector<std::string>{"This is a page about The Mandalorian."});
  events.emplace_back(ConversationEventRole::kUser,
                      ConversationEventType::kPageExcerpt,
                      std::vector<std::string>{"The Mandalorian"});
  events.emplace_back(
      ConversationEventRole::kUser, ConversationEventType::kChatMessage,
      std::vector<std::string>{"Est-ce lié à une série plus large?"});

  // Two tool use requests from the assistant
  events.emplace_back(
      ConversationEventRole::kAssistant, ConversationEventType::kChatMessage,
      std::vector<std::string>{"Going to use a tool..."}, "", std::nullopt,
      MakeToolUseEvents({mojom::ToolUseEvent::New("get_weather", "123",
                                                  "{\"location\":\"New York\"}",
                                                  std::nullopt, nullptr),
                         mojom::ToolUseEvent::New("get_screenshot", "456",
                                                  "{\"type\":\"tab\"}",
                                                  std::nullopt, nullptr)}));

  // First answer from a tool
  events.emplace_back(
      ConversationEventRole::kTool, ConversationEventType::kToolUse,
      MakeContentBlocks(
          {mojom::ContentBlock::NewTextContentBlock(
               mojom::TextContentBlock::New(
                   "The temperature in New York is 60 degrees.")),
           mojom::ContentBlock::NewTextContentBlock(
               mojom::TextContentBlock::New(
                   "The wind in New York is 5 mph from the SW."))}),
      "", std::nullopt, MakeToolUseEvents({}), "123");

  // Second answer from a tool
  events.emplace_back(
      ConversationEventRole::kTool, ConversationEventType::kToolUse,
      MakeContentBlocks({mojom::ContentBlock::NewImageContentBlock(
          mojom::ImageContentBlock::New(
              GURL("data:image/png;base64,R0lGODlhAQABAIAAAAAAAP///"
                   "yH5BAEAAAAALAAAAAABAAEAAAIBRAA7")))}),
      "", std::nullopt, MakeToolUseEvents({}), "456");

  events.emplace_back(
      ConversationEventRole::kUser,
      ConversationEventType::kGetSuggestedTopicsForFocusTabs,
      std::vector<std::string>{"GetSuggestedTopicsForFocusTabs"});
  events.emplace_back(ConversationEventRole::kUser,
                      ConversationEventType::kDedupeTopics,
                      std::vector<std::string>{"DedupeTopics"});
  events.emplace_back(ConversationEventRole::kUser,
                      ConversationEventType::kGetFocusTabsForTopic,
                      std::vector<std::string>{"GetFocusTabsForTopics"}, "C++");
  events.emplace_back(
      ConversationEventRole::kUser, ConversationEventType::kUploadImage,
      std::vector<std::string>{"data:image/png;base64,R0lGODlhAQABAIAAAAAAAP///"
                               "yH5BAEAAAAALAAAAAABAAEAAAIBRAA7",
                               "data:image/png;base64,R0lGODlhAQABAIAAAAAAAP///"
                               "yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"});

  const std::string expected_events_body = R"([
    {
      "role": "user",
      "type": "userMemory",
      "content": "",
      "memory": {"name": "Jane", "memories": ["memory1", "memory2"]}
    },
    {
      "role": "user",
      "type": "pageText",
      "content": "This is a page about The Mandalorian."
    },
    {
      "role": "user",
      "type": "pageExcerpt",
      "content": "The Mandalorian"
    },
    {
      "role": "user",
      "type": "chatMessage",
      "content": "Est-ce lié à une série plus large?"
    },
    {
      "role": "assistant",
      "type": "toolCalls",
      "content": "Going to use a tool...",
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
      "type": "toolUse",
      "content": [
        {
          "type": "text",
          "text": "The temperature in New York is 60 degrees."
        },
        {
          "type": "text",
          "text": "The wind in New York is 5 mph from the SW."
        }
      ],
      "tool_call_id": "123"
    },
    {
      "role": "tool",
      "type": "toolUse",
      "content": [
        {
          "type": "image_url",
          "image_url": {
            "url": "data:image/png;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"
          }
        }
      ],
      "tool_call_id": "456"
    },
    {
      "role": "user",
      "type": "suggestFocusTopics",
      "content": "GetSuggestedTopicsForFocusTabs"
    },
    {
      "role": "user",
      "type": "dedupeFocusTopics",
      "content": "DedupeTopics"
    },
    {
      "role": "user",
      "type": "classifyTabs",
      "content": "GetFocusTabsForTopics",
      "topic": "C++"
    },
    {
      "role": "user",
      "type": "uploadImage",
      "content": [
        "data:image/png;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7",
        "data:image/png;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"
      ]
    }
  ])";

  return std::make_pair(std::move(events), expected_events_body);
}

class MockCallbacks {
 public:
  MOCK_METHOD(void, OnDataReceived, (EngineConsumer::GenerationResultData));
  MOCK_METHOD(void, OnCompleted, (EngineConsumer::GenerationResult));
};

// Mock the AIChatCredentialManager to provide premium credentials
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
class TestConversationAPIClient : public ConversationAPIClient {
 public:
  explicit TestConversationAPIClient(
      AIChatCredentialManager* credential_manager,
      ModelService* model_service)
      : ConversationAPIClient("unit_test_model_name",
                              nullptr,
                              credential_manager,
                              model_service) {
    SetAPIRequestHelperForTesting(std::make_unique<MockAPIRequestHelper>(
        net::NetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
        nullptr));
  }
  ~TestConversationAPIClient() override = default;

  MockAPIRequestHelper* GetMockAPIRequestHelper() {
    return static_cast<MockAPIRequestHelper*>(GetAPIRequestHelperForTesting());
  }
};

}  // namespace

using ConversationEvent = ConversationAPIClient::ConversationEvent;

class ConversationAPIUnitTest : public testing::Test {
 public:
  ConversationAPIUnitTest() = default;
  ~ConversationAPIUnitTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(prefs_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());
    credential_manager_ = std::make_unique<MockAIChatCredentialManager>(
        base::NullCallback(), &prefs_);
    model_service_ = std::make_unique<ModelService>(&prefs_);
    client_ = std::make_unique<TestConversationAPIClient>(
        credential_manager_.get(), model_service_.get());
    // Intercept credential fetch
    ON_CALL(*credential_manager_, FetchPremiumCredential(_))
        .WillByDefault(
            [&](base::OnceCallback<void(std::optional<CredentialCacheEntry>)>
                    callback) {
              std::move(callback).Run(std::move(credential_));
            });
  }

  void TearDown() override {}

  base::Value::List& GetEvents(base::Value::Dict& body) {
    base::Value::List* events = body.FindList("events");
    EXPECT_TRUE(events);
    return *events;
  }

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
  std::unique_ptr<TestConversationAPIClient> client_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::optional<CredentialCacheEntry> credential_ = std::nullopt;
};

TEST_F(ConversationAPIUnitTest, PerformRequest_PremiumHeaders) {
  // Tests the request building part of the ConversationAPIClient:
  //  - headers are set correctly when premium credentials are available
  //  - ConversationEvent is correctly formatted into JSON
  //  - completion response is parsed and passed through to the callbacks
  std::string expected_crediential = "unit_test_credential";
  auto events_and_body = GetMockEventsAndExpectedEventsBody();
  std::vector<ConversationAPIClient::ConversationEvent> events =
      std::move(events_and_body.first);
  std::string expected_events_body = events_and_body.second;
  std::string expected_system_language = "en_KY";
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale(
      expected_system_language);
  std::string expected_completion_response = "Yes, Star Wars";
  std::string expected_selected_language = "fr";
  std::string expected_capability = "chat";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  // Intercept credential fetch and provide premium credentials
  credential_ = CredentialCacheEntry{expected_crediential,
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
        // Verify headers are premium
        auto cookie_header = headers.find("Cookie");
        EXPECT_NE(cookie_header, headers.end());
        EXPECT_EQ(cookie_header->second,
                  "__Secure-sku#brave-leo-premium=" + expected_crediential);
        EXPECT_NE(headers.find("x-brave-key"), headers.end());

        base::Value::Dict body_dict = base::test::ParseJsonDict(body);
        EXPECT_TRUE(!body_dict.empty());

        // Verify input body contains input events in expected json format
        EXPECT_THAT(expected_events_body,
                    base::test::IsJson(GetEvents(body_dict)));

        // Verify body contains the language
        auto [system_language, selected_language] = GetLanguage(body_dict);
        EXPECT_EQ(system_language, expected_system_language);
        EXPECT_TRUE(selected_language.has_value());
        EXPECT_TRUE(selected_language.value().empty());

        // Verify body contains the capability
        const std::string* capability = body_dict.FindString("capability");
        EXPECT_TRUE(capability);
        EXPECT_EQ(*capability, expected_capability);

        // Send some event responses so that we can verify they are passed
        // through to the PerformRequest callbacks as events.
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "isSearching");
          result.GetDict().Set("model", "chat-claude-sonnet");
          data_received_callback.Run(base::ok(std::move(result)));
        }
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "searchQueries");
          result.GetDict().Set("model", "chat-claude-sonnet");
          base::Value queries(base::Value::Type::LIST);
          queries.GetList().Append("Star Wars");
          queries.GetList().Append("Star Trek");
          result.GetDict().Set("queries", std::move(queries));
          data_received_callback.Run(base::ok(std::move(result)));
        }
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "webSources");
          result.GetDict().Set("model", "chat-claude-sonnet");
          base::Value sources(base::Value::Type::LIST);
          {
            // Invalid because it doesn't contain the expected host
            base::Value query(base::Value::Type::DICT);
            query.GetDict().Set("title", "Star Wars");
            query.GetDict().Set("url", "https://starwars.com");
            query.GetDict().Set("favicon", "https://starwars.com/favicon");
            sources.GetList().Append(std::move(query));
          }
          {
            // Invalid because it doesn't contain the expected scheme
            base::Value query(base::Value::Type::DICT);
            query.GetDict().Set("title", "Star Wars");
            query.GetDict().Set("url", "https://starwars.com");
            query.GetDict().Set(
                "favicon", "http://imgs.search.brave.com/starwars.com/favicon");
            sources.GetList().Append(std::move(query));
          }
          {
            // Valid
            base::Value query(base::Value::Type::DICT);
            query.GetDict().Set("title", "Star Wars");
            query.GetDict().Set("url", "https://starwars.com");
            query.GetDict().Set(
                "favicon",
                "https://imgs.search.brave.com/starwars.com/favicon");
            sources.GetList().Append(std::move(query));
          }
          {
            // Valid
            base::Value query(base::Value::Type::DICT);
            query.GetDict().Set("title", "Star Trek");
            query.GetDict().Set("url", "https://startrek.com");
            query.GetDict().Set(
                "favicon",
                "https://imgs.search.brave.com/startrek.com/favicon");
            sources.GetList().Append(std::move(query));
          }
          result.GetDict().Set("sources", std::move(sources));
          data_received_callback.Run(base::ok(std::move(result)));
        }
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "completion");
          result.GetDict().Set("model", "chat-claude-sonnet");
          result.GetDict().Set("completion", expected_completion_response);
          data_received_callback.Run(base::ok(std::move(result)));
        }
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "selectedLanguage");
          result.GetDict().Set("model", "chat-claude-sonnet");
          result.GetDict().Set("language", expected_selected_language);
          data_received_callback.Run(base::ok(std::move(result)));
        }

        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, {}, {}, net::OK,
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
        EXPECT_TRUE(result.event->is_search_status_event());
        EXPECT_TRUE(result.event->get_search_status_event()->is_searching);
      });
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_search_queries_event());
        auto queries = result.event->get_search_queries_event()->search_queries;
        EXPECT_EQ(queries.size(), 2u);
        EXPECT_EQ(queries[0], "Star Wars");
        EXPECT_EQ(queries[1], "Star Trek");
      });
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_sources_event());
        auto& sources = result.event->get_sources_event()->sources;
        EXPECT_EQ(sources.size(), 2u);
        EXPECT_EQ(sources[0]->title, "Star Wars");
        EXPECT_EQ(sources[1]->title, "Star Trek");
        EXPECT_EQ(sources[0]->url.spec(), "https://starwars.com/");
        EXPECT_EQ(sources[1]->url.spec(), "https://startrek.com/");
        EXPECT_EQ(sources[0]->favicon_url.spec(),
                  "https://imgs.search.brave.com/starwars.com/favicon");
        EXPECT_EQ(sources[1]->favicon_url.spec(),
                  "https://imgs.search.brave.com/startrek.com/favicon");
      });
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_completion_event());
        EXPECT_EQ(result.event->get_completion_event()->completion,
                  expected_completion_response);
      });
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_selected_language_event());
        EXPECT_EQ(
            result.event->get_selected_language_event()->selected_language,
            expected_selected_language);
      });
  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](EngineConsumer::GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result->event);
        EXPECT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result.value(),
                  EngineConsumer::GenerationResultData(
                      mojom::ConversationEntryEvent::NewCompletionEvent(
                          mojom::CompletionEvent::New("")),
                      std::nullopt));
        EXPECT_FALSE(result->is_near_verified.has_value());
      });

  // Begin request
  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
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
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIUnitTest, PerformRequest_NonPremium) {
  // Performs the same test as Premium, verifying that nothing else changes
  // apart from request headers (and request url).
  // Tests the request building part of the ConversationAPIClient:
  //  - headers are set correctly when premium credentials are available
  //  - ConversationEvent is correctly formatted into JSON
  //  - completion response is parsed and passed through to the callbacks
  std::string expected_crediential = "unit_test_credential";
  auto events_and_body = GetMockEventsAndExpectedEventsBody();
  std::vector<ConversationAPIClient::ConversationEvent> events =
      std::move(events_and_body.first);
  std::string expected_events_body = events_and_body.second;
  std::string expected_system_language = "en_KY";
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale(
      expected_system_language);
  std::string expected_completion_response = "Yes, Star Wars";
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
        // Verify headers are not premium
        EXPECT_EQ(headers.find("Cookie"), headers.end());
        EXPECT_NE(headers.find("x-brave-key"), headers.end());

        // Verify body contains events in expected json format
        base::Value::Dict body_dict = base::test::ParseJsonDict(body);
        EXPECT_THAT(expected_events_body,
                    base::test::IsJson(GetEvents(body_dict)));

        // Verify body contains the language
        auto [system_language, selected_language] = GetLanguage(body_dict);
        EXPECT_EQ(system_language, expected_system_language);
        EXPECT_TRUE(selected_language.has_value());
        EXPECT_TRUE(selected_language.value().empty());

        // Verify body contains the capability
        const std::string* capability = body_dict.FindString("capability");
        EXPECT_TRUE(capability);
        EXPECT_EQ(*capability, expected_capability);

        // Send a simple completion response so that we can verify it is passed
        // through to the PerformRequest callbacks.
        {
          base::Value result(base::Value::Type::DICT);
          base::Value::Dict& result_dict = result.GetDict();
          result_dict.Set("type", "completion");
          result_dict.Set("model", "llama-3-8b-instruct");
          result_dict.Set("completion", expected_completion_response);
          data_received_callback.Run(base::ok(std::move(result)));
        }

        // Send a selected language event
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "selectedLanguage");
          result.GetDict().Set("model", "llama-3-8b-instruct");
          result.GetDict().Set("language", expected_selected_language);
          data_received_callback.Run(base::ok(std::move(result)));
        }

        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, {}, {}, net::OK,
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
                  expected_completion_response);
      });
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        EXPECT_TRUE(result.event->is_selected_language_event());
        EXPECT_EQ(
            result.event->get_selected_language_event()->selected_language,
            expected_selected_language);
      });
  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([](EngineConsumer::GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(),
                  EngineConsumer::GenerationResultData(
                      mojom::ConversationEntryEvent::NewCompletionEvent(
                          mojom::CompletionEvent::New("")),
                      std::nullopt));
      });

  // Begin request
  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
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

TEST_F(ConversationAPIUnitTest, PerformRequest_WithToolUseResponse) {
  // Tests that we interpret tool use reponses. For more variants
  // see tests for `ToolUseEventFromToolCallsResponse`.
  std::vector<ConversationAPIClient::ConversationEvent> events =
      GetMockEventsAndExpectedEventsBody().first;

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
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "completion");
          result.GetDict().Set("model", "model-1");
          result.GetDict().Set("completion", "This is a test completion");
          result.GetDict().Set("tool_calls", base::test::ParseJsonList(R"([
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
            ])"));
          data_received_callback.Run(base::ok(std::move(result)));
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
        EXPECT_MOJOM_EQ(result.event->get_tool_use_event(),
                        mojom::ToolUseEvent::New("get_weather", "call_123",
                                                 "{\"location\":\"New York\"}",
                                                 std::nullopt, nullptr));
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
                                     std::nullopt, nullptr));
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](const EngineConsumer::GenerationResult& result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->event);
        ASSERT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion, "");
        EXPECT_FALSE(result->model_key.has_value());
      });

  // The payload of the request is not important for this test
  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
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

TEST_F(ConversationAPIUnitTest, PerformRequest_PermissionChallenge) {
  // Tests that we correctly parse alignment_check from the API response
  // and populate the PermissionChallenge in the first ToolUseEvent
  std::vector<ConversationAPIClient::ConversationEvent> events =
      GetMockEventsAndExpectedEventsBody().first;
  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::NiceMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, RequestSSE)
      .WillOnce(testing::WithArgs<4, 5>(
          [&](DataReceivedCallback data_received_callback,
              ResultCallback result_callback) {
            data_received_callback.Run(base::ok(base::test::ParseJson(R"({
              "type": "completion",
              "model": "model-1",
              "completion": "This is a test completion",
              "alignment_check": {
                "allowed": false,
                "reasoning": "Server determined this tool use is off-topic"
              },
              "tool_calls": [
                {
                  "id": "call_123",
                  "type": "function",
                  "function": {
                    "name": "search_web",
                    "arguments": "{\"query\":\"Hello, world!\"}"
                  }
                },
                {
                  "id": "call_456",
                  "type": "function",
                  "function": {
                    "name": "get_weather",
                    "arguments": "{\"location\":\"New York\"}"
                  }
                }
              ]
            })")));

            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(200, {}, {}, net::OK,
                                                          GURL()));
            run_loop.QuitWhenIdle();
            return Ticket();
          }));

  auto expected_tool_use_event_1 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "search_web", "call_123", "{\"query\":\"Hello, world!\"}",
          std::nullopt,
          mojom::PermissionChallenge::New(
              "Server determined this tool use is off-topic", std::nullopt)));

  auto expected_tool_use_event_2 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_456", "{\"location\":\"New York\"}",
          std::nullopt, nullptr));

  // This test is focused on the correctness of the ToolUseEvent,
  // we can leave verifying other events are also sent in another test.
  EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(testing::AnyNumber());

  EXPECT_CALL(mock_callbacks,
              OnDataReceived(testing::Field(
                  "event", &EngineConsumer::GenerationResultData::event,
                  MojomEq(expected_tool_use_event_1.get()))))
      .Times(1);

  EXPECT_CALL(mock_callbacks,
              OnDataReceived(testing::Field(
                  "event", &EngineConsumer::GenerationResultData::event,
                  MojomEq(expected_tool_use_event_2.get()))))
      .Times(1);

  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
      std::nullopt /* oai_tool_definitions */,
      std::nullopt /* preferred_tool_name */,
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
}

TEST_F(ConversationAPIUnitTest, PerformRequest_PermissionChallenge_Allowed) {
  // Tests that we correctly parse alignment_check from the API response
  // and don't populate the PermissionChallenge when allowed is true.
  std::vector<ConversationAPIClient::ConversationEvent> events =
      GetMockEventsAndExpectedEventsBody().first;
  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::NiceMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, RequestSSE)
      .WillOnce(testing::WithArgs<4, 5>(
          [&](DataReceivedCallback data_received_callback,
              ResultCallback result_callback) {
            data_received_callback.Run(base::ok(base::test::ParseJson(R"({
              "type": "completion",
              "model": "model-1",
              "completion": "This is a test completion",
              "alignment_check": {
                "allowed": true,
                "reasoning": "Server determined this tool use is ok"
              },
              "tool_calls": [
                {
                  "id": "call_123",
                  "type": "function",
                  "function": {
                    "name": "search_web",
                    "arguments": "{\"query\":\"Hello, world!\"}"
                  }
                },
                {
                  "id": "call_456",
                  "type": "function",
                  "function": {
                    "name": "get_weather",
                    "arguments": "{\"location\":\"New York\"}"
                  }
                }
              ]
            })")));

            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(200, {}, {}, net::OK,
                                                          GURL()));
            run_loop.QuitWhenIdle();
            return Ticket();
          }));

  auto expected_tool_use_event_1 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "search_web", "call_123", "{\"query\":\"Hello, world!\"}",
          std::nullopt, nullptr));

  auto expected_tool_use_event_2 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_456", "{\"location\":\"New York\"}",
          std::nullopt, nullptr));

  // This test is focused on the correctness of the ToolUseEvent,
  // we can leave verifying other events are also sent in another test.
  EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(testing::AnyNumber());

  EXPECT_CALL(mock_callbacks,
              OnDataReceived(testing::Field(
                  "event", &EngineConsumer::GenerationResultData::event,
                  MojomEq(expected_tool_use_event_1.get()))))
      .Times(1);

  EXPECT_CALL(mock_callbacks,
              OnDataReceived(testing::Field(
                  "event", &EngineConsumer::GenerationResultData::event,
                  MojomEq(expected_tool_use_event_2.get()))))
      .Times(1);

  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
      std::nullopt /* oai_tool_definitions */,
      std::nullopt /* preferred_tool_name */,
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
}

TEST_F(ConversationAPIUnitTest,
       PerformRequest_PermissionChallenge_MissingAllowed) {
  // Tests that we handle unknown alignment_check schema (missing allowed
  // property) by ignoring the alignment_check.
  std::vector<ConversationAPIClient::ConversationEvent> events =
      GetMockEventsAndExpectedEventsBody().first;
  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::NiceMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, RequestSSE)
      .WillOnce(testing::WithArgs<4, 5>(
          [&](DataReceivedCallback data_received_callback,
              ResultCallback result_callback) {
            data_received_callback.Run(base::ok(base::test::ParseJson(R"({
              "type": "completion",
              "model": "model-1",
              "completion": "This is a test completion",
              "alignment_check": {
                "reasoning": "Format unknown"
              },
              "tool_calls": [
                {
                  "id": "call_123",
                  "type": "function",
                  "function": {
                    "name": "search_web",
                    "arguments": "{\"query\":\"Hello, world!\"}"
                  }
                },
                {
                  "id": "call_456",
                  "type": "function",
                  "function": {
                    "name": "get_weather",
                    "arguments": "{\"location\":\"New York\"}"
                  }
                }
              ]
            })")));

            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(200, {}, {}, net::OK,
                                                          GURL()));
            run_loop.QuitWhenIdle();
            return Ticket();
          }));

  auto expected_tool_use_event_1 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "search_web", "call_123", "{\"query\":\"Hello, world!\"}",
          std::nullopt, nullptr));

  auto expected_tool_use_event_2 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_456", "{\"location\":\"New York\"}",
          std::nullopt, nullptr));

  // This test is focused on the correctness of the ToolUseEvent,
  // we can leave verifying other events are also sent in another test.
  EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(testing::AnyNumber());

  EXPECT_CALL(mock_callbacks,
              OnDataReceived(testing::Field(
                  "event", &EngineConsumer::GenerationResultData::event,
                  MojomEq(expected_tool_use_event_1.get()))))
      .Times(1);

  EXPECT_CALL(mock_callbacks,
              OnDataReceived(testing::Field(
                  "event", &EngineConsumer::GenerationResultData::event,
                  MojomEq(expected_tool_use_event_2.get()))))
      .Times(1);

  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
      std::nullopt /* oai_tool_definitions */,
      std::nullopt /* preferred_tool_name */,
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
}

TEST_F(ConversationAPIUnitTest,
       PerformRequest_PermissionChallenge_MissingReasoning) {
  // Tests that we ignore missing reasoning property and still provide
  // PermissionChallenge.
  std::vector<ConversationAPIClient::ConversationEvent> events =
      GetMockEventsAndExpectedEventsBody().first;
  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::NiceMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, RequestSSE)
      .WillOnce(testing::WithArgs<4, 5>(
          [&](DataReceivedCallback data_received_callback,
              ResultCallback result_callback) {
            data_received_callback.Run(base::ok(base::test::ParseJson(R"({
              "type": "completion",
              "model": "model-1",
              "completion": "This is a test completion",
              "alignment_check": {
                "allowed": false,
                "some_other_property": "some_value"
              },
              "tool_calls": [
                {
                  "id": "call_123",
                  "type": "function",
                  "function": {
                    "name": "search_web",
                    "arguments": "{\"query\":\"Hello, world!\"}"
                  }
                },
                {
                  "id": "call_456",
                  "type": "function",
                  "function": {
                    "name": "get_weather",
                    "arguments": "{\"location\":\"New York\"}"
                  }
                }
              ]
            })")));

            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(200, {}, {}, net::OK,
                                                          GURL()));
            run_loop.QuitWhenIdle();
            return Ticket();
          }));

  auto expected_tool_use_event_1 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "search_web", "call_123", "{\"query\":\"Hello, world!\"}",
          std::nullopt,
          mojom::PermissionChallenge::New(std::nullopt, std::nullopt)));

  auto expected_tool_use_event_2 =
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_456", "{\"location\":\"New York\"}",
          std::nullopt, nullptr));

  // This test is focused on the correctness of the ToolUseEvent,
  // we can leave verifying other events are also sent in another test.
  EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(testing::AnyNumber());

  EXPECT_CALL(mock_callbacks, OnDataReceived(testing::Field(
                                  &EngineConsumer::GenerationResultData::event,
                                  MojomEq(expected_tool_use_event_1.get()))))
      .Times(1);

  EXPECT_CALL(mock_callbacks, OnDataReceived(testing::Field(
                                  &EngineConsumer::GenerationResultData::event,
                                  MojomEq(expected_tool_use_event_2.get()))))
      .Times(1);

  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
      std::nullopt /* oai_tool_definitions */,
      std::nullopt /* preferred_tool_name */,
      mojom::ConversationCapability::CHAT,
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
}

TEST_F(ConversationAPIUnitTest,
       PerformRequest_WithModelNameOverride_Streaming) {
  // Tests that the model name override is correctly passed to the API
  std::vector<ConversationAPIClient::ConversationEvent> events =
      GetMockEventsAndExpectedEventsBody().first;
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
        auto dict = base::JSONReader::ReadDict(
            body, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
        EXPECT_TRUE(dict.has_value());
        const std::string* model = dict->FindString("model");
        EXPECT_TRUE(model);
        EXPECT_EQ(*model, override_model_name);

        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "completion");
          result.GetDict().Set("model", override_model_name);
          result.GetDict().Set("completion", "This is a test completion");
          data_received_callback.Run(base::ok(std::move(result)));
        }

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
        ASSERT_TRUE(result->event);
        ASSERT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion, "");
        EXPECT_FALSE(result->model_key.has_value());
      });

  // Begin request with model override
  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
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
}

TEST_F(ConversationAPIUnitTest,
       PerformRequest_WithModelNameOverride_NonStreaming) {
  // Tests that the non-streaming version (Request) is called with null callback
  std::vector<ConversationAPIClient::ConversationEvent> events =
      GetMockEventsAndExpectedEventsBody().first;
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
      .WillOnce([&](const std::string& method, const GURL& url,
                    const std::string& body, const std::string& content_type,
                    ResultCallback result_callback,
                    const base::flat_map<std::string, std::string>& headers,
                    const api_request_helper::APIRequestOptions& options,
                    ResponseConversionCallback response_conversion_callback) {
        // Verify the model name was overridden in the request
        auto dict = base::JSONReader::ReadDict(
            body, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
        EXPECT_TRUE(dict.has_value());
        const std::string* model = dict->FindString("model");
        EXPECT_TRUE(model);
        EXPECT_EQ(*model, override_model_name);

        // Create a response with both completion and model information
        base::Value response(base::Value::Type::DICT);
        response.GetDict().Set("type", "completion");
        response.GetDict().Set("completion", "This is a test completion");
        response.GetDict().Set("model", override_model_name);

        // Complete the request
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, std::move(response),
                                                      {}, net::OK, GURL()));
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
  client_->PerformRequest(std::move(events), "" /* selected_language */,
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
}

TEST_F(ConversationAPIUnitTest, PerformRequest_NEARVerification) {
  std::string expected_completion_response = "Verified response";
  auto events_and_body = GetMockEventsAndExpectedEventsBody();
  std::vector<ConversationAPIClient::ConversationEvent> events =
      std::move(events_and_body.first);

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
        base::Value result(base::Value::Type::DICT);
        result.GetDict().Set("type", "completion");
        result.GetDict().Set("model", "llama-3-8b-instruct");
        result.GetDict().Set("completion", expected_completion_response);
        data_received_callback.Run(base::ok(std::move(result)));

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
        EXPECT_TRUE(result.value().is_near_verified.has_value());
        EXPECT_TRUE(result.value().is_near_verified.value());
      });

  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
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
}

TEST_F(ConversationAPIUnitTest, FailNoConversationEvents) {
  // Tests handling invalid request parameters
  std::vector<ConversationAPIClient::ConversationEvent> events;

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;

  // Intercept API Request Helper call and verify the request is as expected
  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .Times(0);

  // Callbacks should be passed through and translated from APIRequestHelper
  // format.
  EXPECT_CALL(mock_callbacks, OnDataReceived).Times(0);
  EXPECT_CALL(
      mock_callbacks,
      OnCompleted(testing::Eq(base::unexpected(mojom::APIError::None))));

  // Begin request
  client_->PerformRequest(
      std::move(events), "" /* selected_language */,
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

TEST_F(ConversationAPIUnitTest, ParseResponseEvent_ParsesContentReceiptEvent) {
  base::Value::Dict content_receipt_event;
  content_receipt_event.Set("type", "contentReceipt");
  content_receipt_event.Set("model", "llama-3-8b-instruct");
  content_receipt_event.Set("total_tokens", static_cast<int>(1234567890));
  content_receipt_event.Set("trimmed_tokens", static_cast<int>(987654321));

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(content_receipt_event,
                                                model_service_.get());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_content_receipt_event());
  EXPECT_EQ(result->event->get_content_receipt_event()->total_tokens,
            1234567890u);
  EXPECT_EQ(result->event->get_content_receipt_event()->trimmed_tokens,
            987654321u);
  EXPECT_EQ(result->model_key, "chat-basic");

  // Test with missing values (both missing)
  // Should default to 0 when values are missing
  base::Value::Dict missing_values_event;
  missing_values_event.Set("type", "contentReceipt");
  missing_values_event.Set("model", "llama-3-8b-instruct");
  result = ConversationAPIClient::ParseResponseEvent(missing_values_event,
                                                     model_service_.get());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_content_receipt_event());
  EXPECT_EQ(result->event->get_content_receipt_event()->total_tokens, 0u);
  EXPECT_EQ(result->event->get_content_receipt_event()->trimmed_tokens, 0u);
  EXPECT_EQ(result->model_key, "chat-basic");

  // Test with missing trimmed_tokens only
  base::Value::Dict missing_trimmed_event;
  missing_trimmed_event.Set("type", "contentReceipt");
  missing_trimmed_event.Set("model", "llama-3-8b-instruct");
  missing_trimmed_event.Set("total_tokens", static_cast<int>(12345));

  // No trimmed_tokens set
  result = ConversationAPIClient::ParseResponseEvent(missing_trimmed_event,
                                                     model_service_.get());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_content_receipt_event());
  EXPECT_EQ(result->event->get_content_receipt_event()->total_tokens, 12345u);
  EXPECT_EQ(result->event->get_content_receipt_event()->trimmed_tokens, 0u);
  EXPECT_EQ(result->model_key, "chat-basic");

  // Test with negative values
  base::Value::Dict negative_values_event;
  negative_values_event.Set("type", "contentReceipt");
  negative_values_event.Set("model", "llama-3-8b-instruct");
  negative_values_event.Set("total_tokens", static_cast<int>(-100));
  negative_values_event.Set("trimmed_tokens", static_cast<int>(-200));
  result = ConversationAPIClient::ParseResponseEvent(negative_values_event,
                                                     model_service_.get());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_content_receipt_event());
  // Should default to 0 for negative values
  EXPECT_EQ(result->event->get_content_receipt_event()->total_tokens, 0u);
  EXPECT_EQ(result->event->get_content_receipt_event()->trimmed_tokens, 0u);
  EXPECT_EQ(result->model_key, "chat-basic");

  // Test with mixed values (one positive, one negative)
  base::Value::Dict mixed_values_event;
  mixed_values_event.Set("type", "contentReceipt");
  mixed_values_event.Set("model", "llama-3-8b-instruct");
  mixed_values_event.Set("total_tokens", static_cast<int>(500));
  mixed_values_event.Set("trimmed_tokens", static_cast<int>(-50));
  result = ConversationAPIClient::ParseResponseEvent(mixed_values_event,
                                                     model_service_.get());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_content_receipt_event());
  EXPECT_EQ(result->event->get_content_receipt_event()->total_tokens, 500u);
  EXPECT_EQ(result->event->get_content_receipt_event()->trimmed_tokens, 0u);
  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest, ParseResponseEvent_ParsesCompletionEvent) {
  base::Value::Dict completion_event;
  completion_event.Set("type", "completion");
  completion_event.Set("model", "llama-3-8b-instruct");
  completion_event.Set("completion", "Wherever I go, he goes");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(completion_event,
                                                model_service_.get());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_completion_event());
  EXPECT_EQ(result->event->get_completion_event()->completion,
            "Wherever I go, he goes");
  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest, ParseResponseEvent_ParsesIsSearchingEvent) {
  base::Value::Dict is_searching_event;
  is_searching_event.Set("type", "isSearching");
  is_searching_event.Set("model", "llama-3-8b-instruct");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(is_searching_event,
                                                model_service_.get());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_search_status_event());
  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest, ParseResponseEvent_ParsesSearchQueriesEvent) {
  base::Value::Dict search_queries_event;
  search_queries_event.Set("type", "searchQueries");
  search_queries_event.Set("model", "llama-3-8b-instruct");

  base::Value::List queries;
  queries.Append("query1");
  queries.Append("query2");
  search_queries_event.Set("queries", std::move(queries));

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(search_queries_event,
                                                model_service_.get());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_search_queries_event());
  EXPECT_EQ(result->event->get_search_queries_event()->search_queries.size(),
            2u);
  EXPECT_EQ(result->event->get_search_queries_event()->search_queries[0],
            "query1");
  EXPECT_EQ(result->event->get_search_queries_event()->search_queries[1],
            "query2");
  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_ParsesConversationTitleEvent) {
  base::Value::Dict conversation_title_event;
  conversation_title_event.Set("type", "conversationTitle");
  conversation_title_event.Set("model", "llama-3-8b-instruct");
  conversation_title_event.Set("title", "This is the way");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(conversation_title_event,
                                                model_service_.get());
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_conversation_title_event());
  EXPECT_EQ(result->event->get_conversation_title_event()->title,
            "This is the way");
  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest, ParseResponseEvent_ParsesWebSourcesEvent) {
  // Case 1: Valid favicon from allowed brave host
  base::Value::Dict event_with_valid_favicon;
  event_with_valid_favicon.Set("type", "webSources");
  event_with_valid_favicon.Set("model", "llama-3-8b-instruct");
  base::Value::List sources1;

  base::Value::Dict source1;
  source1.Set("title", "Example 1");
  source1.Set("url", "https://example.com/1");
  source1.Set("favicon", "https://imgs.search.brave.com/favicon.ico");
  sources1.Append(std::move(source1));

  event_with_valid_favicon.Set("sources", std::move(sources1));

  std::optional<EngineConsumer::GenerationResultData> result1 =
      ConversationAPIClient::ParseResponseEvent(event_with_valid_favicon,
                                                model_service_.get());
  ASSERT_TRUE(result1);
  ASSERT_TRUE(result1->event);
  ASSERT_TRUE(result1->event->is_sources_event());
  EXPECT_EQ(result1->event->get_sources_event()->sources.size(), 1u);
  EXPECT_EQ(result1->event->get_sources_event()->sources[0]->title,
            "Example 1");
  EXPECT_EQ(result1->event->get_sources_event()->sources[0]->url.spec(),
            "https://example.com/1");
  EXPECT_EQ(result1->event->get_sources_event()->sources[0]->favicon_url.spec(),
            "https://imgs.search.brave.com/favicon.ico");
  EXPECT_EQ(result1->model_key, "chat-basic");

  // Case 2: Missing favicon, should use default
  base::Value::Dict event_with_missing_favicon;
  event_with_missing_favicon.Set("type", "webSources");
  event_with_missing_favicon.Set("model", "llama-3-8b-instruct");
  base::Value::List sources2;

  base::Value::Dict source2;
  source2.Set("title", "Example 2");
  source2.Set("url", "https://example.com/2");
  sources2.Append(std::move(source2));

  event_with_missing_favicon.Set("sources", std::move(sources2));

  std::optional<EngineConsumer::GenerationResultData> result2 =
      ConversationAPIClient::ParseResponseEvent(event_with_missing_favicon,
                                                model_service_.get());
  ASSERT_TRUE(result2);
  ASSERT_TRUE(result2->event);
  ASSERT_TRUE(result2->event->is_sources_event());
  EXPECT_EQ(result2->event->get_sources_event()->sources.size(), 1u);
  EXPECT_EQ(result2->event->get_sources_event()->sources[0]->title,
            "Example 2");
  EXPECT_EQ(result2->event->get_sources_event()->sources[0]->url.spec(),
            "https://example.com/2");
  EXPECT_EQ(result2->event->get_sources_event()->sources[0]->favicon_url.spec(),
            "chrome-untrusted://resources/brave-icons/globe.svg");
  EXPECT_EQ(result2->model_key, "chat-basic");

  // Case 3: Disallowed favicon host, should be skipped
  // We manage the allowed list in kAllowedWebSourceFaviconHost
  base::Value::Dict event_with_disallowed_favicon;
  event_with_disallowed_favicon.Set("type", "webSources");
  event_with_disallowed_favicon.Set("model", "llama-3-8b-instruct");
  base::Value::List sources3;

  base::Value::Dict source3;
  source3.Set("title", "Example 3");
  source3.Set("url", "https://example.com/3");
  source3.Set("favicon",
              "https://untrusted.com/favicon.ico");  // disallowed host
  sources3.Append(std::move(source3));

  event_with_disallowed_favicon.Set("sources", std::move(sources3));

  std::optional<EngineConsumer::GenerationResultData> result3 =
      ConversationAPIClient::ParseResponseEvent(event_with_disallowed_favicon,
                                                model_service_.get());
  EXPECT_FALSE(result3) << "Disallowed favicon host should be filtered out";
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_ParsesWebSourcesEventWithRichResults) {
  // Test webSources event with valid rich_results data
  base::Value::Dict event = base::test::ParseJsonDict(R"({
    "type": "webSources",
    "model": "llama-3-8b-instruct",
    "sources": [
      {
        "title": "Example Source",
        "url": "https://example.com",
        "favicon": "https://imgs.search.brave.com/favicon.ico"
      }
    ],
    "rich_results": [
      {
        "results": [
          {
            "type": "knowledge_graph",
            "title": "Knowledge Graph Title",
            "description": "Some description"
          },
          {
            "type": "video",
            "url": "https://video.example.com",
            "thumbnail": "https://imgs.search.brave.com/thumb.jpg"
          }
        ]
      }
    ]
  })");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(event, model_service_.get());

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_sources_event());

  auto& sources_event = result->event->get_sources_event();
  EXPECT_EQ(sources_event->sources.size(), 1u);
  EXPECT_EQ(sources_event->sources[0]->title, "Example Source");

  // Verify rich_results were parsed
  ASSERT_EQ(sources_event->rich_results.size(), 2u);

  // Verify first rich result using IsJson
  EXPECT_THAT(sources_event->rich_results[0], base::test::IsJson(R"({
                "type": "knowledge_graph",
                "title": "Knowledge Graph Title",
                "description": "Some description"
              })"));

  // Verify second rich result using IsJson
  EXPECT_THAT(sources_event->rich_results[1], base::test::IsJson(R"({
                "type": "video",
                "url": "https://video.example.com",
                "thumbnail": "https://imgs.search.brave.com/thumb.jpg"
              })"));

  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_ParsesWebSourcesEventWithMultipleRichResultGroups) {
  // Test webSources event with multiple rich_results groups
  base::Value::Dict event = base::test::ParseJsonDict(R"({
    "type": "webSources",
    "model": "llama-3-8b-instruct",
    "sources": [
      {
        "title": "Example Source",
        "url": "https://example.com"
      }
    ],
    "rich_results": [
      {
        "results": [
          {"id": "group1_item1"}
        ]
      },
      {
        "results": [
          {"id": "group2_item1"},
          {"id": "group2_item2"}
        ]
      }
    ]
  })");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(event, model_service_.get());

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_sources_event());

  auto& sources_event = result->event->get_sources_event();
  // Should have 3 total rich results (1 from group1, 2 from group2)
  ASSERT_EQ(sources_event->rich_results.size(), 3u);

  // Verify each item using IsJson
  EXPECT_THAT(sources_event->rich_results[0],
              base::test::IsJson(R"({"id": "group1_item1"})"));
  EXPECT_THAT(sources_event->rich_results[1],
              base::test::IsJson(R"({"id": "group2_item1"})"));
  EXPECT_THAT(sources_event->rich_results[2],
              base::test::IsJson(R"({"id": "group2_item2"})"));
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_WebSourcesEventWithInvalidRichResults) {
  // Test that invalid rich_results items are skipped gracefully
  // Note: Must construct manually to test invalid structures
  base::Value::Dict event = base::test::ParseJsonDict(R"({
    "type": "webSources",
    "model": "llama-3-8b-instruct",
    "sources": [
      {
        "title": "Example Source",
        "url": "https://example.com"
      }
    ]
  })");

  // Add rich_results with various invalid items
  base::Value::List rich_results;

  // Invalid: not a dict
  rich_results.Append("invalid_string");

  // Invalid: missing "results" key
  rich_results.Append(base::test::ParseJsonDict(R"({"other_key": "value"})"));

  // Valid group
  rich_results.Append(base::test::ParseJsonDict(R"({
    "results": [{"id": "valid_item"}]
  })"));

  // Invalid: results is not a list
  base::Value::Dict invalid_results_group;
  invalid_results_group.Set("results", "not_a_list");
  rich_results.Append(std::move(invalid_results_group));

  // Valid group but with invalid result items mixed in
  base::Value::Dict mixed_group;
  base::Value::List mixed_results;
  mixed_results.Append("invalid_item");  // not a dict
  mixed_results.Append(base::test::ParseJsonDict(R"({"id": "valid_item2"})"));
  mixed_group.Set("results", std::move(mixed_results));
  rich_results.Append(std::move(mixed_group));

  event.Set("rich_results", std::move(rich_results));

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(event, model_service_.get());

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_sources_event());

  auto& sources_event = result->event->get_sources_event();
  // Should only have 2 valid rich results
  ASSERT_EQ(sources_event->rich_results.size(), 2u);

  // Verify the valid items were parsed correctly using IsJson
  EXPECT_THAT(sources_event->rich_results[0],
              base::test::IsJson(R"({"id": "valid_item"})"));
  EXPECT_THAT(sources_event->rich_results[1],
              base::test::IsJson(R"({"id": "valid_item2"})"));
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_WebSourcesEventWithoutRichResults) {
  // Test that webSources event works fine without rich_results
  base::Value::Dict event = base::test::ParseJsonDict(R"({
    "type": "webSources",
    "model": "llama-3-8b-instruct",
    "sources": [
      {
        "title": "Example Source",
        "url": "https://example.com"
      }
    ]
  })");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(event, model_service_.get());

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_sources_event());

  auto& sources_event = result->event->get_sources_event();
  EXPECT_EQ(sources_event->sources.size(), 1u);
  EXPECT_EQ(sources_event->rich_results.size(), 0u);
  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_WebSourcesEventWithEmptyRichResults) {
  // Test that empty rich_results list is handled correctly
  base::Value::Dict event = base::test::ParseJsonDict(R"({
    "type": "webSources",
    "model": "llama-3-8b-instruct",
    "sources": [
      {
        "title": "Example Source",
        "url": "https://example.com"
      }
    ],
    "rich_results": []
  })");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(event, model_service_.get());

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_sources_event());

  auto& sources_event = result->event->get_sources_event();
  EXPECT_EQ(sources_event->sources.size(), 1u);
  EXPECT_EQ(sources_event->rich_results.size(), 0u);
  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_ParsesWebSourcesEventWithInfoBoxes) {
  // Test webSources event with valid info_boxes data
  base::Value::Dict event = base::test::ParseJsonDict(R"({
    "type": "webSources",
    "model": "llama-3-8b-instruct",
    "sources": [
      {
        "title": "Example Source",
        "url": "https://example.com",
        "favicon": "https://imgs.search.brave.com/favicon.ico"
      }
    ],
    "info_boxes": [
      {
        "type": "knowledge_graph",
        "title": "Knowledge Graph Title",
        "description": "Some description"
      },
      {
        "type": "infobox",
        "title": "Info Box Title",
        "data": "Some data"
      }
    ]
  })");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(event, model_service_.get());

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_sources_event());

  auto& sources_event = result->event->get_sources_event();
  EXPECT_EQ(sources_event->sources.size(), 1u);
  EXPECT_EQ(sources_event->sources[0]->title, "Example Source");

  // Verify info_boxes were parsed
  ASSERT_EQ(sources_event->info_boxes.size(), 2u);

  // Verify first info box using IsJson
  EXPECT_THAT(sources_event->info_boxes[0], base::test::IsJson(R"({
                "type": "knowledge_graph",
                "title": "Knowledge Graph Title",
                "description": "Some description"
              })"));

  // Verify second info box using IsJson
  EXPECT_THAT(sources_event->info_boxes[1], base::test::IsJson(R"({
                "type": "infobox",
                "title": "Info Box Title",
                "data": "Some data"
              })"));

  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_WebSourcesEventWithInvalidInfoBoxes) {
  // Test that invalid info_boxes items are skipped gracefully
  // Note: Must construct manually to test invalid structures
  base::Value::Dict event = base::test::ParseJsonDict(R"({
    "type": "webSources",
    "model": "llama-3-8b-instruct",
    "sources": [
      {
        "title": "Example Source",
        "url": "https://example.com"
      }
    ]
  })");

  // Add info_boxes with various invalid items
  base::Value::List info_boxes;

  // Invalid: not a dict
  info_boxes.Append("invalid_string");

  // Valid info box
  info_boxes.Append(base::test::ParseJsonDict(R"({"id": "valid_item1"})"));

  // Invalid: not a dict
  info_boxes.Append(123);

  // Valid info box
  info_boxes.Append(base::test::ParseJsonDict(R"({"id": "valid_item2"})"));

  event.Set("info_boxes", std::move(info_boxes));

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(event, model_service_.get());

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_sources_event());

  auto& sources_event = result->event->get_sources_event();
  // Should only have 2 valid info boxes
  ASSERT_EQ(sources_event->info_boxes.size(), 2u);

  // Verify the valid items were parsed correctly using IsJson
  EXPECT_THAT(sources_event->info_boxes[0],
              base::test::IsJson(R"({"id": "valid_item1"})"));
  EXPECT_THAT(sources_event->info_boxes[1],
              base::test::IsJson(R"({"id": "valid_item2"})"));
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_WebSourcesEventWithoutInfoBoxes) {
  // Test that webSources event works fine without info_boxes
  base::Value::Dict event = base::test::ParseJsonDict(R"({
    "type": "webSources",
    "model": "llama-3-8b-instruct",
    "sources": [
      {
        "title": "Example Source",
        "url": "https://example.com"
      }
    ]
  })");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(event, model_service_.get());

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_sources_event());

  auto& sources_event = result->event->get_sources_event();
  EXPECT_EQ(sources_event->sources.size(), 1u);
  EXPECT_EQ(sources_event->info_boxes.size(), 0u);
  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest,
       ParseResponseEvent_WebSourcesEventWithEmptyInfoBoxes) {
  // Test that empty info_boxes list is handled correctly
  base::Value::Dict event = base::test::ParseJsonDict(R"({
    "type": "webSources",
    "model": "llama-3-8b-instruct",
    "sources": [
      {
        "title": "Example Source",
        "url": "https://example.com"
      }
    ],
    "info_boxes": []
  })");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(event, model_service_.get());

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_sources_event());

  auto& sources_event = result->event->get_sources_event();
  EXPECT_EQ(sources_event->sources.size(), 1u);
  EXPECT_EQ(sources_event->info_boxes.size(), 0u);
  EXPECT_EQ(result->model_key, "chat-basic");
}

TEST_F(ConversationAPIUnitTest, ParseResponseEvent_InvalidEventType) {
  base::Value::Dict invalid_event;
  invalid_event.Set("type", "unknownThisIsTheWayEvent");
  invalid_event.Set("model", "llama-3-8b-instruct");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(invalid_event,
                                                model_service_.get());
  EXPECT_FALSE(result);
}

TEST_F(ConversationAPIUnitTest, ParseResponseEvent_MissingModelKey) {
  base::Value::Dict invalid_event;
  invalid_event.Set("type", "conversationTitle");
  invalid_event.Set("title", "This is the way");

  std::optional<EngineConsumer::GenerationResultData> result =
      ConversationAPIClient::ParseResponseEvent(invalid_event,
                                                model_service_.get());
  EXPECT_FALSE(result);
}

}  // namespace ai_chat
