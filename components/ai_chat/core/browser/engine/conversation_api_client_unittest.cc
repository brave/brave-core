/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ConversationHistory = std::vector<ai_chat::mojom::ConversationTurn>;
using ::testing::_;
using ::testing::Sequence;
using DataReceivedCallback =
    api_request_helper::APIRequestHelper::DataReceivedCallback;
using ResultCallback = api_request_helper::APIRequestHelper::ResultCallback;
using Ticket = api_request_helper::APIRequestHelper::Ticket;

namespace ai_chat {

using ConversationEvent = ConversationAPIClient::ConversationEvent;

class MockCallbacks {
 public:
  MOCK_METHOD(void, OnDataReceived, (mojom::ConversationEntryEventPtr));
  MOCK_METHOD(void, OnCompleted, (EngineConsumer::GenerationResult));
};

// Mock the AIChatCredentialManager to provide premium credentials
class MockAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;
  MOCK_METHOD(void,
              FetchPremiumCredential,
              (base::OnceCallback<void(std::optional<CredentialCacheEntry>)>),
              (override));
};

// Mock the APIRequestHelper to intercept requests and provide responses
class MockAPIRequestHelper : public api_request_helper::APIRequestHelper {
 public:
  using api_request_helper::APIRequestHelper::APIRequestHelper;
  MOCK_METHOD(Ticket,
              RequestSSE,
              (const std::string&,
               const GURL&,
               const std::string&,
               const std::string&,
               DataReceivedCallback,
               ResultCallback,
               (const base::flat_map<std::string, std::string>&),
               const api_request_helper::APIRequestOptions&),
              (override));
};

// Create a version of the ConversationAPIClient that contains our mocks
class TestConversationAPIClient : public ConversationAPIClient {
 public:
  explicit TestConversationAPIClient(
      AIChatCredentialManager* credential_manager)
      : ConversationAPIClient("unit_test_model_name",
                              nullptr,
                              credential_manager) {
    SetAPIRequestHelperForTesting(std::make_unique<MockAPIRequestHelper>(
        net::NetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
        nullptr));
  }
  ~TestConversationAPIClient() override = default;

  MockAPIRequestHelper* GetMockAPIRequestHelper() {
    return static_cast<MockAPIRequestHelper*>(GetAPIRequestHelperForTesting());
  }
};

class ConversationAPIUnitTest : public testing::Test {
 public:
  ConversationAPIUnitTest() = default;
  ~ConversationAPIUnitTest() override = default;

  void SetUp() override {
    credential_manager_ = std::make_unique<MockAIChatCredentialManager>(
        base::NullCallback(), &prefs_service_);
    client_ =
        std::make_unique<TestConversationAPIClient>(credential_manager_.get());
    // Intercept credential fetch
    EXPECT_CALL(*credential_manager_, FetchPremiumCredential(_))
        .WillOnce(
            [&](base::OnceCallback<void(std::optional<CredentialCacheEntry>)>
                    callback) {
              std::move(callback).Run(std::move(credential_));
            });
  }

  void TearDown() override {}

  std::string GetEventsJson(std::string_view body_json) {
    auto dict = base::JSONReader::ReadDict(body_json);
    EXPECT_TRUE(dict.has_value());
    base::Value::List* events = dict->FindList("events");
    EXPECT_TRUE(events);
    std::string events_json;
    base::JSONWriter::WriteWithOptions(
        *events, base::JSONWriter::OPTIONS_PRETTY_PRINT, &events_json);
    return events_json;
  }

  // Returns a pair of system_language and selected_langauge
  // The system language is the OS locale.
  // The selected language is the language the server side determined the
  // conversation is in
  std::pair<std::string, std::optional<std::string>> GetLanguage(
      std::string_view body_json) {
    auto dict = base::JSONReader::ReadDict(body_json);
    EXPECT_TRUE(dict.has_value());
    if (!dict.has_value()) {
      return {"", std::nullopt};
    }

    const std::string* system_language = dict->FindString("system_language");
    // The system language should always be present
    EXPECT_TRUE(system_language != nullptr);

    const std::string* selected_language =
        dict->FindString("selected_language");
    if (selected_language) {
      return {*system_language, *selected_language};
    } else {
      return {*system_language, std::nullopt};
    }
  }

  std::string FormatComparableEventsJson(std::string_view formatted_json) {
    auto events = base::JSONReader::Read(formatted_json);
    EXPECT_TRUE(events.has_value()) << "Verify that the string is valid JSON!";
    std::string events_json;
    base::JSONWriter::WriteWithOptions(
        events.value(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &events_json);
    return events_json;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestConversationAPIClient> client_;
  std::unique_ptr<MockAIChatCredentialManager> credential_manager_;
  TestingPrefServiceSimple prefs_service_;
  std::optional<CredentialCacheEntry> credential_ = std::nullopt;
};

TEST_F(ConversationAPIUnitTest, PerformRequest_PremiumHeaders) {
  // Tests the request building part of the ConversationAPIClient:
  //  - headers are set correctly when premium credentials are available
  //  - ConversationEvent is correctly formatted into JSON
  //  - completion response is parsed and passed through to the callbacks
  std::string expected_crediential = "unit_test_credential";
  std::vector<ConversationAPIClient::ConversationEvent> events = {
      {mojom::CharacterType::HUMAN, ConversationAPIClient::PageText,
       "This is a page about The Mandalorian."},
      {mojom::CharacterType::HUMAN, ConversationAPIClient::PageExcerpt,
       "The Mandalorian"},
      {mojom::CharacterType::HUMAN, ConversationAPIClient::ChatMessage,
       "Est-ce lié à une série plus large?"}};
  std::string expected_events_body = R"([
    {"role": "user", "type": "pageText", "content": "This is a page about The Mandalorian."},
    {"role": "user", "type": "pageExcerpt", "content": "The Mandalorian"},
    {"role": "user", "type": "chatMessage", "content": "Est-ce lié à une série plus large?"}
  ])";
  std::string expected_system_language = "en_KY";
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale(
      expected_system_language);
  std::string expected_completion_response = "Yes, Star Wars";
  std::string expected_selected_language = "fr";

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

        // Verify body contains events in expected json format
        EXPECT_STREQ(GetEventsJson(body).c_str(),
                     FormatComparableEventsJson(expected_events_body).c_str());

        // Verify body contains the language
        auto [system_language, selected_language] = GetLanguage(body);
        EXPECT_EQ(system_language, expected_system_language);
        EXPECT_TRUE(selected_language.has_value());
        EXPECT_TRUE(selected_language.value().empty());

        // Send some event responses so that we can verify it is passed
        // through to the PerformRequest callbacks.
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "isSearching");
          data_received_callback.Run(base::ok(std::move(result)));
        }
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "searchQueries");
          base::Value queries(base::Value::Type::LIST);
          queries.GetList().Append("Star Wars");
          queries.GetList().Append("Star Trek");
          result.GetDict().Set("queries", std::move(queries));
          data_received_callback.Run(base::ok(std::move(result)));
        }
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "completion");
          result.GetDict().Set("completion", expected_completion_response);
          data_received_callback.Run(base::ok(std::move(result)));
        }
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "selectedLanguage");
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
      .WillOnce([&](mojom::ConversationEntryEventPtr event) {
        EXPECT_TRUE(event->is_search_status_event());
        EXPECT_TRUE(event->get_search_status_event()->is_searching);
      });
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](mojom::ConversationEntryEventPtr event) {
        EXPECT_TRUE(event->is_search_queries_event());
        auto queries = event->get_search_queries_event()->search_queries;
        EXPECT_EQ(queries.size(), 2u);
        EXPECT_EQ(queries[0], "Star Wars");
        EXPECT_EQ(queries[1], "Star Trek");
      });
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](mojom::ConversationEntryEventPtr event) {
        EXPECT_TRUE(event->is_completion_event());
        EXPECT_EQ(event->get_completion_event()->completion,
                  expected_completion_response);
      });
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](mojom::ConversationEntryEventPtr event) {
        EXPECT_TRUE(event->is_selected_language_event());
        EXPECT_EQ(event->get_selected_language_event()->selected_language,
                  expected_selected_language);
      });
  EXPECT_CALL(mock_callbacks,
              OnCompleted(EngineConsumer::GenerationResult("")));

  // Begin request
  client_->PerformRequest(
      events, "",
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
  std::vector<ConversationAPIClient::ConversationEvent> events = {
      {mojom::CharacterType::HUMAN, ConversationAPIClient::PageText,
       "This is a page about The Mandalorian."},
      {mojom::CharacterType::HUMAN, ConversationAPIClient::PageExcerpt,
       "The Mandalorian"},
      {mojom::CharacterType::HUMAN, ConversationAPIClient::ChatMessage,
       "Est-ce lié à une série plus large?"}};
  std::string expected_events_body = R"([
    {"role": "user", "type": "pageText", "content": "This is a page about The Mandalorian."},
    {"role": "user", "type": "pageExcerpt", "content": "The Mandalorian"},
    {"role": "user", "type": "chatMessage", "content": "Est-ce lié à une série plus large?"}
  ])";
  std::string expected_system_language = "en_KY";
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale(
      expected_system_language);
  std::string expected_completion_response = "Yes, Star Wars";
  std::string expected_selected_language = "fr";

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
        EXPECT_STREQ(GetEventsJson(body).c_str(),
                     FormatComparableEventsJson(expected_events_body).c_str());

        // Verify body contains the language
        auto [system_language, selected_language] = GetLanguage(body);
        EXPECT_EQ(system_language, expected_system_language);
        EXPECT_TRUE(selected_language.has_value());
        EXPECT_TRUE(selected_language.value().empty());

        // Send a simple completion response so that we can verify it is passed
        // through to the PerformRequest callbacks.
        {
          base::Value result(base::Value::Type::DICT);
          base::Value::Dict& result_dict = result.GetDict();
          result_dict.Set("type", "completion");
          result_dict.Set("completion", expected_completion_response);
          data_received_callback.Run(base::ok(std::move(result)));
        }

        // Send a selected language event
        {
          base::Value result(base::Value::Type::DICT);
          result.GetDict().Set("type", "selectedLanguage");
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
      .WillOnce([&](mojom::ConversationEntryEventPtr event) {
        EXPECT_TRUE(event->is_completion_event());
        EXPECT_EQ(event->get_completion_event()->completion,
                  expected_completion_response);
      });
  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](mojom::ConversationEntryEventPtr event) {
        EXPECT_TRUE(event->is_selected_language_event());
        EXPECT_EQ(event->get_selected_language_event()->selected_language,
                  expected_selected_language);
      });
  EXPECT_CALL(mock_callbacks,
              OnCompleted(EngineConsumer::GenerationResult("")));

  // Begin request
  client_->PerformRequest(
      events, "",
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

TEST_F(ConversationAPIUnitTest, FailNoConversationEvents) {
  // Tests handling invalid request parameters
  std::vector<ConversationAPIClient::ConversationEvent> events;
  EngineConsumer::GenerationResult expected_result =
      base::unexpected(mojom::APIError::None);

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;

  // Intercept API Request Helper call and verify the request is as expected
  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .Times(0);

  // Callbacks should be passed through and translated from APIRequestHelper
  // format.
  EXPECT_CALL(mock_callbacks, OnDataReceived).Times(0);
  EXPECT_CALL(mock_callbacks, OnCompleted(expected_result));

  // Begin request
  client_->PerformRequest(
      events, "",
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(credential_manager_.get());
}

}  // namespace ai_chat
