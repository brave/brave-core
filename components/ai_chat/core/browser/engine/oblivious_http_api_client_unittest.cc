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
              /*endpoint_url=*/GURL("https://endpoint.test/inner")});
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
        "data: {\"choices\":[{\"delta\":{\"content\":\"%s\"}}]}", content));
  }

  void CompleteWithInnerResponse(int response_code, std::string body) {
    ASSERT_TRUE(completion_client_.is_bound());
    auto inner = network::mojom::ObliviousHttpResponse::New();
    inner->response_code = response_code;
    inner->response_body = std::move(body);
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
            if (data.event->is_completion_event()) {
              received_chunks_ +=
                  data.event->get_completion_event()->completion;
            }
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
  std::string received_chunks_;
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

  EXPECT_EQ("part1part2", received_chunks_);

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
  EmitRawChunk("this is not valid json or sse");
  EmitChunk("good2");
  EmitRawChunk("{\"also\": \"garbage\"}");
  EmitChunk("good3");

  CompleteWithInnerResponse(net::HTTP_OK, "");
  run_loop_->Run();

  EXPECT_EQ("good1good2good3", received_chunks_);
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

TEST_F(ObliviousHttpAPIClientUnitTest, PerformRequest_BadOuterResponseCode) {
  PerformRequest();

  ASSERT_TRUE(completion_client_.is_bound());
  completion_client_->OnCompleted(
      network::mojom::ObliviousHttpCompletionResult::NewOuterResponseErrorCode(
          net::HTTP_INTERNAL_SERVER_ERROR));
  completion_client_.FlushForTesting();
  run_loop_->Run();

  ASSERT_FALSE(result_.has_value());
  EXPECT_EQ(mojom::APIError::ConnectionIssue, result_.error());
}

}  // namespace ai_chat
