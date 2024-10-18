/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"

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
#include "base/strings/string_util.h"
#include "base/test/task_environment.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
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
using GenerationResult = ai_chat::OAIAPIClient::GenerationResult;

namespace ai_chat {

class MockCallbacks {
 public:
  MOCK_METHOD(void, OnDataReceived, (mojom::ConversationEntryEventPtr));
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
    SetAPIRequestHelperForTesting(std::make_unique<MockAPIRequestHelper>(
        net::NetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
        nullptr));
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

  void TearDown() override {}

  std::string GetMessagesJson(std::string_view body_json) {
    auto dict = base::JSONReader::ReadDict(body_json);
    EXPECT_TRUE(dict.has_value());
    base::Value::List* events = dict->FindList("messages");
    EXPECT_TRUE(events);
    std::string events_json;
    base::JSONWriter::WriteWithOptions(
        *events, base::JSONWriter::OPTIONS_PRETTY_PRINT, &events_json);
    return events_json;
  }

  std::string FormatComparableEventsJson(std::string_view formatted_json) {
    auto messages = base::JSONReader::Read(formatted_json);
    EXPECT_TRUE(messages.has_value())
        << "Verify that the string is valid JSON!";
    std::string json;
    base::JSONWriter::WriteWithOptions(
        messages.value(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
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
        EXPECT_STREQ(
            GetMessagesJson(body).c_str(),
            FormatComparableEventsJson(expected_conversation_body).c_str());

        auto chunk = base::JSONReader::Read(server_chunk);
        EXPECT_TRUE(chunk.has_value());
        data_received_callback.Run(base::ok(std::move(chunk.value())));

        auto completed = base::JSONReader::Read(server_completion);
        EXPECT_TRUE(completed.has_value());
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(
                200, std::move(completed.value()), {}, net::OK, GURL()));

        run_loop.Quit();
        return Ticket();
      });

  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .WillOnce([&](mojom::ConversationEntryEventPtr event) {
        EXPECT_TRUE(event->is_completion_event());
        EXPECT_EQ(event->get_completion_event()->completion,
                  expected_chunk_response);
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](GenerationResult result) {
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), expected_completion_response);
      });

  // Begin request
  auto messages = base::JSONReader::Read(expected_conversation_body);
  EXPECT_TRUE(messages.has_value());

  client_->PerformRequest(
      *model_options, std::move(messages.value().GetList()),
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client_.get());
  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
}

}  // namespace ai_chat
