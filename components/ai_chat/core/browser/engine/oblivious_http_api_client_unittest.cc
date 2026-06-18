// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oblivious_http_api_client.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/oblivious_http_config_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"
#include "services/network/test/test_network_context.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ::testing::_;

namespace ai_chat {

namespace {

constexpr char kTestModelName[] = "test-model";
constexpr char kTestUpstreamModelName[] = "upstream-test-model";

// JSON body for a non-streaming OAI completion response.
constexpr char kNonStreamingResponseBody[] =
    R"({"choices":[{"message":{"content":"hello world"}}]})";

}  // namespace

class MockAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;
  ~MockAIChatCredentialManager() override = default;

  MOCK_METHOD(void,
              FetchPremiumCredential,
              (base::OnceCallback<void(std::optional<CredentialCacheEntry>)>),
              (override));
};

class MockObliviousHttpConfigManager : public ObliviousHttpConfigManager {
 public:
  explicit MockObliviousHttpConfigManager(PrefService* prefs)
      : ObliviousHttpConfigManager(/*url_loader_factory=*/nullptr, prefs) {}
  ~MockObliviousHttpConfigManager() override = default;

  MOCK_METHOD(void,
              RequestKeyConfig,
              (const std::string& model_name, KeyConfigCallback callback),
              (override));
};

class ObliviousHttpAPIClientUnitTest : public testing::Test,
                                       public network::TestNetworkContext {
 protected:
  void SetUp() override {
    prefs::RegisterProfilePrefs(prefs_.registry());

    credential_manager_ = std::make_unique<MockAIChatCredentialManager>(
        base::NullCallback(), &prefs_);
    ON_CALL(*credential_manager_, FetchPremiumCredential(_))
        .WillByDefault(
            [](base::OnceCallback<void(std::optional<CredentialCacheEntry>)>
                   cb) { std::move(cb).Run(std::nullopt); });

    client_ = std::make_unique<ObliviousHttpAPIClient>(
        /*url_loader_factory=*/nullptr,
        base::BindLambdaForTesting(
            [this]() -> network::mojom::NetworkContext* { return this; }),
        credential_manager_.get(), &prefs_);

    auto config_manager =
        std::make_unique<MockObliviousHttpConfigManager>(&prefs_);
    EXPECT_CALL(*config_manager, RequestKeyConfig(_, _))
        .Times(testing::AtLeast(1))
        .WillRepeatedly([](const std::string&,
                           ObliviousHttpConfigManager::KeyConfigCallback cb) {
          std::move(cb).Run(ObliviousHttpConfigManager::KeyConfigResult{
              /*key_config=*/"test-key-config-bytes",
              /*endpoint_url=*/GURL("https://endpoint.test/inner"),
              /*upstream_model_name=*/kTestUpstreamModelName});
        });
    client_->SetConfigManagerForTesting(std::move(config_manager));
  }

  void EmitRawChunk(const std::string& raw) {
    ASSERT_TRUE(chunk_client_.is_bound());
    chunk_client_->OnBodyChunk(raw);
    chunk_client_.FlushForTesting();
  }

  void EmitChunk(const std::string& content) {
    EmitRawChunk(absl::StrFormat(
        "data: {\"choices\":[{\"delta\":{\"content\":\"%s\"}}]}\n", content));
  }

  void CompleteWithInnerResponse(int response_code, std::string body) {
    ASSERT_TRUE(completion_client_.is_bound());
    auto inner = network::mojom::ObliviousHttpResponse::New();
    inner->response_code = response_code;
    inner->response_body = std::move(body);
    inner->headers =
        net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK\r\n");
    completion_client_->OnCompleted(
        network::mojom::ObliviousHttpCompletionResult::NewInnerResponse(
            std::move(inner)));
    completion_client_.FlushForTesting();
  }

  void PerformRequest(bool enable_data_received_callback = false) {
    auto leo = mojom::LeoModelOptions::New();
    leo->name = kTestModelName;
    auto model_options =
        mojom::ModelOptions::NewLeoModelOptions(std::move(leo));

    OAIMessage msg;
    msg.role = "user";
    msg.content.push_back(mojom::ContentBlock::NewTextContentBlock(
        mojom::TextContentBlock::New("Hello")));

    EngineConsumer::GenerationDataCallback data_received_callback;
    if (enable_data_received_callback) {
      data_received_callback = base::BindLambdaForTesting(
          [&](EngineConsumer::GenerationResultData data) {
            received_chunks_.push_back(std::move(data.event));
          });
    }

    run_loop_ = std::make_unique<base::RunLoop>();
    std::vector<OAIMessage> messages;
    messages.push_back(std::move(msg));
    client_->PerformRequest(
        *model_options, std::move(messages), std::nullopt,
        std::move(data_received_callback),
        base::BindLambdaForTesting([&](EngineConsumer::GenerationResult r) {
          result_ = std::move(r);
          run_loop_->Quit();
        }));
  }

  // network::TestNetworkContext:
  void GetViaObliviousHttp(
      network::mojom::ObliviousHttpRequestPtr request,
      mojo::PendingRemote<network::mojom::ObliviousHttpClient> client)
      override {
    last_request_ = std::move(request);
    EXPECT_NE(std::string::npos, last_request_->relay_url.spec().find(
                                     "v1/models/test-model/relay"));
    completion_client_.Bind(std::move(client));

    if (last_request_->enable_chunking &&
        last_request_->chunk_client.is_valid()) {
      chunk_client_.Bind(std::move(last_request_->chunk_client));
    }
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<MockAIChatCredentialManager> credential_manager_;
  std::unique_ptr<ObliviousHttpAPIClient> client_;
  std::unique_ptr<base::RunLoop> run_loop_;
  EngineConsumer::GenerationResult result_ =
      base::unexpected(mojom::APIError::None);
  std::vector<mojom::ConversationEntryEventPtr> received_chunks_;
  network::mojom::ObliviousHttpRequestPtr last_request_;
  mojo::Remote<network::mojom::ObliviousHttpClient> completion_client_;
  mojo::Remote<network::mojom::ObliviousHttpChunkClient> chunk_client_;
};

TEST_F(ObliviousHttpAPIClientUnitTest, PerformRequest_NonStreaming_Completes) {
  PerformRequest();

  ASSERT_FALSE(last_request_.is_null());
  EXPECT_FALSE(last_request_->enable_chunking);
  EXPECT_FALSE(last_request_->chunk_client.is_valid());

  CompleteWithInnerResponse(net::HTTP_OK, kNonStreamingResponseBody);
  run_loop_->Run();

  ASSERT_TRUE(result_.has_value());
  ASSERT_TRUE(result_->event->is_completion_event());
  EXPECT_EQ("hello world", result_->event->get_completion_event()->completion);
}

TEST_F(ObliviousHttpAPIClientUnitTest,
       PerformRequest_Streaming_DeliversChunksAndCompletion) {
  PerformRequest(/*enable_data_received_callback=*/true);

  ASSERT_FALSE(last_request_.is_null());
  EXPECT_TRUE(last_request_->enable_chunking);

  EmitChunk("part1");
  EmitChunk("part2");

  CompleteWithInnerResponse(net::HTTP_OK, "");
  run_loop_->Run();

  ASSERT_EQ(2u, received_chunks_.size());
  ASSERT_TRUE(received_chunks_[0]->is_completion_event());
  EXPECT_EQ("part1", received_chunks_[0]->get_completion_event()->completion);
  ASSERT_TRUE(received_chunks_[1]->is_completion_event());
  EXPECT_EQ("part2", received_chunks_[1]->get_completion_event()->completion);

  ASSERT_TRUE(result_.has_value());
  ASSERT_TRUE(result_->event->is_completion_event());
  EXPECT_EQ("", result_->event->get_completion_event()->completion);
}

TEST_F(ObliviousHttpAPIClientUnitTest,
       PerformRequest_Streaming_SkipsMalformedChunks) {
  PerformRequest(/*enable_data_received_callback=*/true);

  ASSERT_FALSE(last_request_.is_null());
  EXPECT_TRUE(last_request_->enable_chunking);

  EmitChunk("good1");
  EmitRawChunk("this is not valid json or sse\n");
  EmitChunk("good2");
  EmitRawChunk("{\"also\": \"garbage\"}\n");
  EmitChunk("good3");

  CompleteWithInnerResponse(net::HTTP_OK, "");
  run_loop_->Run();

  ASSERT_EQ(3u, received_chunks_.size());
  ASSERT_TRUE(received_chunks_[0]->is_completion_event());
  EXPECT_EQ("good1", received_chunks_[0]->get_completion_event()->completion);
  ASSERT_TRUE(received_chunks_[1]->is_completion_event());
  EXPECT_EQ("good2", received_chunks_[1]->get_completion_event()->completion);
  ASSERT_TRUE(received_chunks_[2]->is_completion_event());
  EXPECT_EQ("good3", received_chunks_[2]->get_completion_event()->completion);
}

TEST_F(ObliviousHttpAPIClientUnitTest,
       PerformRequest_NonStreaming_BadStatusCode) {
  PerformRequest();

  CompleteWithInnerResponse(net::HTTP_INTERNAL_SERVER_ERROR, "");
  run_loop_->Run();

  ASSERT_FALSE(result_.has_value());
  EXPECT_EQ(mojom::APIError::ConnectionIssue, result_.error());
}

TEST_F(ObliviousHttpAPIClientUnitTest, PerformRequest_Streaming_BadStatusCode) {
  PerformRequest(/*enable_data_received_callback=*/true);

  ASSERT_FALSE(last_request_.is_null());
  EXPECT_TRUE(last_request_->enable_chunking);

  EmitChunk("part1");
  EmitChunk("part2");

  CompleteWithInnerResponse(net::HTTP_INTERNAL_SERVER_ERROR, "");
  run_loop_->Run();

  ASSERT_FALSE(result_.has_value());
  EXPECT_EQ(mojom::APIError::ConnectionIssue, result_.error());
}

TEST_F(ObliviousHttpAPIClientUnitTest,
       PerformRequest_InnerUnauthorized_ReturnsConnectionIssue) {
  PerformRequest();
  CompleteWithInnerResponse(net::HTTP_UNAUTHORIZED, "");
  run_loop_->Run();
  ASSERT_FALSE(result_.has_value());
  EXPECT_EQ(mojom::APIError::ConnectionIssue, result_.error());
}

TEST_F(ObliviousHttpAPIClientUnitTest,
       PerformRequest_Streaming_ToolCallAndNearAIResult) {
  PerformRequest(/*enable_data_received_callback=*/true);

  ASSERT_FALSE(last_request_.is_null());
  EXPECT_TRUE(last_request_->enable_chunking);

  // Emit a web_context_search tool call request.
  EmitRawChunk(
      "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{"
      "\"id\":\"call_abc\","
      "\"function\":{\"name\":\"web_context_search\","
      "\"arguments\":\"{\\\"query\\\":\\\"brave browser\\\"}\"}"
      "}]}}]}\n");

  // Emit a nearai server tool result for the same tool call.
  EmitRawChunk(
      "data: {\"choices\":[{\"delta\":{\"nearai_tool_result\":{"
      "\"tool_call_id\":\"call_abc\","
      "\"output\":\"Brave is a privacy-focused browser.\""
      "}}}]}\n");

  CompleteWithInnerResponse(net::HTTP_OK, "");
  run_loop_->Run();

  ASSERT_EQ(2u, received_chunks_.size());

  // First event: tool call request.
  ASSERT_TRUE(received_chunks_[0]->is_tool_use_event());
  const auto& tool_call = received_chunks_[0]->get_tool_use_event();
  EXPECT_EQ("web_context_search", tool_call->tool_name);
  EXPECT_EQ("call_abc", tool_call->id);
  EXPECT_EQ("{\"query\":\"brave browser\"}", tool_call->arguments_json);
  EXPECT_FALSE(tool_call->is_server_result);

  // Second event: nearai server tool result.
  ASSERT_TRUE(received_chunks_[1]->is_tool_use_event());
  const auto& tool_result = received_chunks_[1]->get_tool_use_event();
  EXPECT_EQ("call_abc", tool_result->id);
  EXPECT_TRUE(tool_result->is_server_result);
  ASSERT_TRUE(tool_result->output.has_value());
  ASSERT_EQ(1u, tool_result->output->size());
  ASSERT_TRUE((*tool_result->output)[0]->is_text_content_block());
  EXPECT_EQ("Brave is a privacy-focused browser.",
            (*tool_result->output)[0]->get_text_content_block()->text);
}

TEST_F(ObliviousHttpAPIClientUnitTest, PerformRequest_BadOuterResponseCode) {
  const struct {
    int response_code;
    mojom::APIError expected_error;
  } kCases[] = {
      {net::HTTP_INTERNAL_SERVER_ERROR, mojom::APIError::ConnectionIssue},
      {net::HTTP_UNAUTHORIZED, mojom::APIError::ConnectionIssue},
      {net::HTTP_TOO_MANY_REQUESTS, mojom::APIError::RateLimitReached},
  };

  for (const auto& c : kCases) {
    completion_client_.reset();

    PerformRequest();

    ASSERT_TRUE(completion_client_.is_bound());
    completion_client_->OnCompleted(
        network::mojom::ObliviousHttpCompletionResult::
            NewOuterResponseErrorCode(c.response_code));
    completion_client_.FlushForTesting();
    run_loop_->Run();

    ASSERT_FALSE(result_.has_value());
    EXPECT_EQ(c.expected_error, result_.error());
  }
}

TEST_F(ObliviousHttpAPIClientUnitTest, PerformRequest_UsesUpstreamModelName) {
  PerformRequest();

  ASSERT_FALSE(last_request_.is_null());
  ASSERT_TRUE(last_request_->request_body);

  auto parsed = base::JSONReader::Read(last_request_->request_body->content,
                                       base::JSON_PARSE_RFC);
  ASSERT_TRUE(parsed.has_value() && parsed->is_dict());
  const std::string* model = parsed->GetDict().FindString("model");
  ASSERT_TRUE(model);
  EXPECT_EQ(kTestUpstreamModelName, *model);
}

}  // namespace ai_chat
