/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ollama/ollama_service.h"

#include <memory>
#include <string>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/ollama.mojom.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kOllamaSuccessResponse[] = "TEST";
constexpr char kOllamaModelsResponse[] = R"({
  "models": [
    {
      "name": "llama2:7b",
      "modified_at": "2024-01-01T00:00:00Z",
      "size": 3825819519
    },
    {
      "name": "mistral:latest",
      "modified_at": "2024-01-02T00:00:00Z",
      "size": 4109865159
    }
  ]
})";

}  // namespace

class OllamaServiceTest : public testing::Test {
 public:
  OllamaServiceTest() = default;
  ~OllamaServiceTest() override = default;

  void SetUp() override {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);
    ollama_client_ = std::make_unique<OllamaService>(shared_url_loader_factory_,
                                                     /*model_fetcher=*/nullptr);
  }

  OllamaService* ollama_client() { return ollama_client_.get(); }
  network::TestURLLoaderFactory* test_url_loader_factory() {
    return &test_url_loader_factory_;
  }

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<OllamaService> ollama_client_;
};

TEST_F(OllamaServiceTest, ConnectedSuccess) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaBaseUrl,
                                         kOllamaSuccessResponse);

  base::test::TestFuture<bool> future;
  ollama_client()->IsConnected(future.GetCallback());
  EXPECT_TRUE(future.Get());
}

TEST_F(OllamaServiceTest, ConnectedFailure) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaBaseUrl, "",
                                         net::HTTP_INTERNAL_SERVER_ERROR);

  base::test::TestFuture<bool> future;
  ollama_client()->IsConnected(future.GetCallback());
  EXPECT_FALSE(future.Get());
}

TEST_F(OllamaServiceTest, ConnectedNoResponse) {
  // Simulate network error with failed response
  test_url_loader_factory()->AddResponse(
      GURL(ai_chat::mojom::kOllamaBaseUrl),
      network::mojom::URLResponseHead::New(), "",
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED));

  base::test::TestFuture<bool> future;
  ollama_client()->IsConnected(future.GetCallback());
  EXPECT_FALSE(future.Get());
}

TEST_F(OllamaServiceTest, ConnectedWrongResponse) {
  // Any 200 response from Ollama base URL indicates it's running
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaBaseUrl,
                                         "Some other response");

  base::test::TestFuture<bool> future;
  ollama_client()->IsConnected(future.GetCallback());
  EXPECT_TRUE(future.Get());
}

TEST_F(OllamaServiceTest, FetchModelsSuccess) {
  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaListModelsAPIEndpoint, kOllamaModelsResponse);

  base::test::TestFuture<std::optional<std::vector<std::string>>> future;
  ollama_client()->FetchModels(future.GetCallback());

  const auto& models = future.Get();
  ASSERT_TRUE(models.has_value());
  ASSERT_EQ(2u, models->size());
  EXPECT_EQ("llama2:7b", (*models)[0]);
  EXPECT_EQ("mistral:latest", (*models)[1]);
}

TEST_F(OllamaServiceTest, FetchModelsNoResponse) {
  // Simulate network error with failed response
  test_url_loader_factory()->AddResponse(
      GURL(ai_chat::mojom::kOllamaListModelsAPIEndpoint),
      network::mojom::URLResponseHead::New(), "",
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED));

  base::test::TestFuture<std::optional<std::vector<std::string>>> future;
  ollama_client()->FetchModels(future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(OllamaServiceTest, FetchModelsEmptyResponse) {
  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaListModelsAPIEndpoint, "");

  base::test::TestFuture<std::optional<std::vector<std::string>>> future;
  ollama_client()->FetchModels(future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(OllamaServiceTest, FetchModelsInvalidJSON) {
  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaListModelsAPIEndpoint, "{invalid json}");

  base::test::TestFuture<std::optional<std::vector<std::string>>> future;
  ollama_client()->FetchModels(future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(OllamaServiceTest, FetchModelsMissingModelsKey) {
  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaListModelsAPIEndpoint, R"({"other_key": []})");

  base::test::TestFuture<std::optional<std::vector<std::string>>> future;
  ollama_client()->FetchModels(future.GetCallback());
  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(OllamaServiceTest, FetchModelsInvalidModelStructure) {
  constexpr char kInvalidModelsResponse[] = R"({
    "models": [
      {
        "modified_at": "2024-01-01T00:00:00Z",
        "size": 3825819519
      }
    ]
  })";

  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaListModelsAPIEndpoint, kInvalidModelsResponse);

  base::test::TestFuture<std::optional<std::vector<std::string>>> future;
  ollama_client()->FetchModels(future.GetCallback());

  const auto& models = future.Get();
  ASSERT_TRUE(models.has_value());
  EXPECT_EQ(0u, models->size());
}

TEST_F(OllamaServiceTest, ShowModelSuccess) {
  constexpr char kModelDetailsResponse[] = R"({
    "model_info": {
      "general.architecture": "llama",
      "llama.context_length": 4096
    },
    "capabilities": ["vision", "chat"]
  })";

  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaShowModelInfoAPIEndpoint, kModelDetailsResponse);

  base::RunLoop run_loop;
  ollama_client()->ShowModel(
      "llama2:7b", base::BindLambdaForTesting(
                       [&](std::optional<OllamaService::ModelDetails> details) {
                         ASSERT_TRUE(details.has_value());
                         EXPECT_EQ(4096u, details->context_length);
                         EXPECT_TRUE(details->has_vision);
                         run_loop.Quit();
                       }));

  run_loop.Run();
}

TEST_F(OllamaServiceTest, ShowModelNoResponse) {
  test_url_loader_factory()->AddResponse(
      GURL(ai_chat::mojom::kOllamaShowModelInfoAPIEndpoint),
      network::mojom::URLResponseHead::New(), "",
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED));

  base::RunLoop run_loop;
  ollama_client()->ShowModel(
      "llama2:7b", base::BindLambdaForTesting(
                       [&](std::optional<OllamaService::ModelDetails> details) {
                         EXPECT_FALSE(details.has_value());
                         run_loop.Quit();
                       }));

  run_loop.Run();
}

TEST_F(OllamaServiceTest, ShowModelInvalidJSON) {
  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaShowModelInfoAPIEndpoint, "{invalid json}");

  base::RunLoop run_loop;
  ollama_client()->ShowModel(
      "llama2:7b", base::BindLambdaForTesting(
                       [&](std::optional<OllamaService::ModelDetails> details) {
                         EXPECT_FALSE(details.has_value());
                         run_loop.Quit();
                       }));

  run_loop.Run();
}

TEST_F(OllamaServiceTest, ShowModelNoModelInfo) {
  constexpr char kNoModelInfoResponse[] = R"({
    "capabilities": ["vision"]
  })";

  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaShowModelInfoAPIEndpoint, kNoModelInfoResponse);

  base::RunLoop run_loop;
  ollama_client()->ShowModel(
      "llama2:7b", base::BindLambdaForTesting(
                       [&](std::optional<OllamaService::ModelDetails> details) {
                         ASSERT_TRUE(details.has_value());
                         EXPECT_EQ(0u, details->context_length);
                         EXPECT_TRUE(details->has_vision);
                         run_loop.Quit();
                       }));

  run_loop.Run();
}

TEST_F(OllamaServiceTest, ShowModelNoVisionCapability) {
  constexpr char kNoVisionResponse[] = R"({
    "model_info": {
      "llama.context_length": 8192
    },
    "capabilities": ["chat"]
  })";

  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaShowModelInfoAPIEndpoint, kNoVisionResponse);

  base::RunLoop run_loop;
  ollama_client()->ShowModel(
      "llama2:7b", base::BindLambdaForTesting(
                       [&](std::optional<OllamaService::ModelDetails> details) {
                         ASSERT_TRUE(details.has_value());
                         EXPECT_EQ(8192u, details->context_length);
                         EXPECT_FALSE(details->has_vision);
                         run_loop.Quit();
                       }));

  run_loop.Run();
}

TEST_F(OllamaServiceTest, ShowModelEmptyResponse) {
  test_url_loader_factory()->AddResponse(
      ai_chat::mojom::kOllamaShowModelInfoAPIEndpoint, R"({})");

  base::RunLoop run_loop;
  ollama_client()->ShowModel(
      "llama2:7b", base::BindLambdaForTesting(
                       [&](std::optional<OllamaService::ModelDetails> details) {
                         ASSERT_TRUE(details.has_value());
                         EXPECT_EQ(0u, details->context_length);
                         EXPECT_FALSE(details->has_vision);
                         run_loop.Quit();
                       }));

  run_loop.Run();
}

// Direct tests for ParseModelsResponse private method
TEST_F(OllamaServiceTest, ParseModelsResponse_Valid) {
  constexpr char kValidResponse[] = R"({
    "models": [
      {"name": "llama2:7b"},
      {"name": "mistral:latest"}
    ]
  })";

  auto result = ollama_client()->ParseModelsResponse(kValidResponse);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(2u, result->size());
  EXPECT_EQ("llama2:7b", (*result)[0]);
  EXPECT_EQ("mistral:latest", (*result)[1]);
}

TEST_F(OllamaServiceTest, ParseModelsResponse_InvalidJSON) {
  auto result = ollama_client()->ParseModelsResponse("{invalid json}");
  EXPECT_FALSE(result.has_value());
}

TEST_F(OllamaServiceTest, ParseModelsResponse_MissingModelsKey) {
  auto result = ollama_client()->ParseModelsResponse(R"({"other_key": []})");
  EXPECT_FALSE(result.has_value());
}

TEST_F(OllamaServiceTest, ParseModelsResponse_EmptyModels) {
  constexpr char kEmptyResponse[] = R"({"models": []})";

  auto result = ollama_client()->ParseModelsResponse(kEmptyResponse);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result->size());
}

TEST_F(OllamaServiceTest, ParseModelsResponse_InvalidModelStructure) {
  constexpr char kInvalidStructure[] = R"({
    "models": [
      {"modified_at": "2024-01-01T00:00:00Z"},
      {"name": "valid:model"}
    ]
  })";

  auto result = ollama_client()->ParseModelsResponse(kInvalidStructure);
  ASSERT_TRUE(result.has_value());
  // Only the valid model should be included
  ASSERT_EQ(1u, result->size());
  EXPECT_EQ("valid:model", (*result)[0]);
}

// Direct tests for ParseModelDetailsResponse private method
TEST_F(OllamaServiceTest, ParseModelDetailsResponse_Valid) {
  constexpr char kValidResponse[] = R"({
    "model_info": {
      "llama.context_length": 4096
    },
    "capabilities": ["vision", "chat"]
  })";

  auto result = ollama_client()->ParseModelDetailsResponse(kValidResponse);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(4096u, result->context_length);
  EXPECT_TRUE(result->has_vision);
}

TEST_F(OllamaServiceTest, ParseModelDetailsResponse_InvalidJSON) {
  auto result = ollama_client()->ParseModelDetailsResponse("{invalid}");
  EXPECT_FALSE(result.has_value());
}

TEST_F(OllamaServiceTest, ParseModelDetailsResponse_NoModelInfo) {
  constexpr char kNoModelInfo[] = R"({"capabilities": ["vision"]})";

  auto result = ollama_client()->ParseModelDetailsResponse(kNoModelInfo);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result->context_length);
  EXPECT_TRUE(result->has_vision);
}

TEST_F(OllamaServiceTest, ParseModelDetailsResponse_NoCapabilities) {
  constexpr char kNoCapabilities[] = R"({
    "model_info": {
      "llama.context_length": 8192
    }
  })";

  auto result = ollama_client()->ParseModelDetailsResponse(kNoCapabilities);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(8192u, result->context_length);
  EXPECT_FALSE(result->has_vision);
}

TEST_F(OllamaServiceTest, ParseModelDetailsResponse_EmptyResponse) {
  auto result = ollama_client()->ParseModelDetailsResponse(R"({})");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(0u, result->context_length);
  EXPECT_FALSE(result->has_vision);
}

}  // namespace ai_chat
