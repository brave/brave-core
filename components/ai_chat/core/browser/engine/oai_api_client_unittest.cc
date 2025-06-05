/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"

#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ConversationHistory = std::vector<ai_chat::mojom::ConversationTurn>;
using ::testing::_;
using ::testing::Sequence;
using DataReceivedCallback =
    api_request_helper::APIRequestHelper::DataReceivedCallback;
using ResultCallback = api_request_helper::APIRequestHelper::ResultCallback;
using Ticket = api_request_helper::APIRequestHelper::Ticket;
using GenerationResult = ai_chat::OAIAPIClient::GenerationResult;

namespace ai_chat {

namespace {
// A helper method which parses a string_view and returns the JSON or a
// base::Value object if the JSON is invalid.
base::Value ParseOrStringValue(const std::string& json) {
  auto maybe_json = base::JSONReader::Read(json);
  if (!maybe_json.has_value()) {
    return base::Value(json);
  }
  return std::move(maybe_json.value());
}
}  // namespace

class MockCallbacks {
 public:
  MOCK_METHOD(void, OnDataReceived, (EngineConsumer::GenerationResultData));
  MOCK_METHOD(void, OnCompleted, (GenerationResult));
};

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

class TestOAIAPIClient : public OAIAPIClient {
 public:
  TestOAIAPIClient() : OAIAPIClient(nullptr) {
    auto mock_helper =
        std::make_unique<testing::NiceMock<MockAPIRequestHelper>>(
            net::NetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
            nullptr);
    SetAPIRequestHelperForTesting(std::move(mock_helper));
  }
  ~TestOAIAPIClient() override = default;

  MockAPIRequestHelper* GetMockAPIRequestHelper() {
    return static_cast<MockAPIRequestHelper*>(GetAPIRequestHelperForTesting());
  }
};

class OAIAPIUnitTest : public testing::Test {
 public:
  OAIAPIUnitTest() = default;
  ~OAIAPIUnitTest() override = default;

  void SetUp() override { client_ = std::make_unique<TestOAIAPIClient>(); }

  void TearDown() override { client_.reset(); }

  std::string GetMessagesJson(std::string_view body_json) {
    auto dict = base::test::ParseJsonDict(body_json);
    base::Value::List* events = dict.FindList("messages");
    EXPECT_TRUE(events);
    std::string events_json;
    base::JSONWriter::WriteWithOptions(
        *events, base::JSONWriter::OPTIONS_PRETTY_PRINT, &events_json);
    return events_json;
  }

  std::string FormatComparableEventsJson(std::string_view formatted_json) {
    auto messages = base::test::ParseJson(formatted_json);
    std::string json;
    base::JSONWriter::WriteWithOptions(
        messages, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
    return json;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestOAIAPIClient> client_;
};

TEST_F(OAIAPIUnitTest, PerformRequest) {
  mojom::CustomModelOptionsPtr model_options = mojom::CustomModelOptions::New(
      "test_api_key", 0, 0, 0, "test_system_prompt", GURL("https://test.com"),
      "test_model");

  std::string server_chunk =
      R"({"id":"chatcmpl-123","object":"chat.completion.chunk","created":1694268190,"model":"gpt-3.5-turbo-0125", "system_fingerprint": "fp_44709d6fcb", "choices":[{"index":0,"delta":{"role":"assistant","content":"It was played in Arlington, Texas."},"logprobs":null,"finish_reason":null}]})";
  std::string server_completion =
      R"({"id":"chatcmpl-123","object":"chat.completion","created":1677652288,"model":"gpt-3.5-turbo-0125","system_fingerprint":"fp_44709d6fcb","choices":[{"index":0,"message":{"role":"assistant","content":"\n\nCan I assist you further?"},"logprobs":null,"finish_reason":"stop"}],"usage":{"prompt_tokens":9,"completion_tokens":12,"total_tokens":21}})";

  std::string expected_chunk_response = "It was played in Arlington, Texas.";
  std::string expected_completion_response = "\n\nCan I assist you further?";
  std::string expected_conversation_body = R"([
    {"role": "user", "content": "Where was it played?"}
  ])";

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
        EXPECT_EQ(url, model_options->endpoint);
        EXPECT_EQ(headers.contains("Authorization"), true);
        EXPECT_EQ(method, net::HttpRequestHeaders::kPostMethod);
        EXPECT_EQ(GetMessagesJson(body),
                  FormatComparableEventsJson(expected_conversation_body));

        auto chunk = base::test::ParseJson(server_chunk);
        data_received_callback.Run(base::ok(std::move(chunk)));

        auto completed = base::test::ParseJson(server_completion);
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, std::move(completed),
                                                      {}, net::OK, GURL()));

        run_loop.Quit();
        return Ticket();
      });

  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        ASSERT_TRUE(result.event->is_completion_event());
        EXPECT_EQ(result.event->get_completion_event()->completion,
                  expected_chunk_response);
        EXPECT_FALSE(result.model_key.has_value());
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->event);
        ASSERT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion,
                  expected_completion_response);
        EXPECT_FALSE(result->model_key.has_value());
      });

  // Begin request
  auto messages = base::test::ParseJsonList(expected_conversation_body);

  client_->PerformRequest(
      *model_options, std::move(messages),
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();

  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
}

class OAIAPIInvalidResponseTest
    : public OAIAPIUnitTest,
      public ::testing::WithParamInterface<std::string> {};

TEST_P(OAIAPIInvalidResponseTest,
       InvalidResponse_NoCallbacksTriggeredOrEmptyCompletion) {
  mojom::CustomModelOptionsPtr model_options = mojom::CustomModelOptions::New(
      "test_api_key", 0, 0, 0, "test_system_prompt", GURL("https://test.com"),
      "test_model");

  const std::string invalid_server_response = GetParam();

  base::RunLoop run_loop;
  testing::StrictMock<MockCallbacks> mock_callbacks;
  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();

  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .WillOnce([&](auto, auto, auto, auto,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback, auto, auto) {
        // Simulate data chunk received
        base::Value maybe_val = ParseOrStringValue(invalid_server_response);
        data_received_callback.Run(base::ok(std::move(maybe_val)));

        // Simulate final callback
        base::Value maybe_val_final =
            ParseOrStringValue(invalid_server_response);
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(
                200, std::move(maybe_val_final), {}, net::OK, GURL()));

        run_loop.Quit();
        return Ticket();
      });

  // For invalid payloads, we expect no callbacks from OnDataReceived
  EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(0);

  // For invalid 200 OK payloads, we expect an empty completion from OnCompleted
  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([](GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->event);
        ASSERT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion, "");
      });

  // Begin request
  client_->PerformRequest(
      *model_options, base::Value::List(),
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();

  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
}

// A set of invalid responses that should not trigger any callbacks
INSTANTIATE_TEST_SUITE_P(
    OAIAPIInvalidResponseScenarios,
    OAIAPIInvalidResponseTest,
    ::testing::Values(
        // aaaaaaaaaaaaaaa
        "aaaaaaaaaaaaaaaaa",
        // {"invalid": "json"}
        R"({"invalid": "json"})",
        // {choices: []}
        R"({"choices": []})",
        // {"choices": [{"message": {"content": []}]}
        R"({"choices": [{"message": {"content": []}}]})",
        // Empty JSON object
        R"({})",
        // Malformed JSON
        R"({"choices": [)",
        // Unexpected data types
        R"({"choices": "unexpected_string"})",
        // Nested invalid JSON
        R"({"choices": [{"message": {"content": {"nested": "invalid"}}}]})",
        // Valid JSON with missing fields
        R"({"choices": [{"index": 0}]})"));

}  // namespace ai_chat
