/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ollama/ollama_client.h"

#include <memory>
#include <string>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/ollama.mojom.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kOllamaSuccessResponse[] = "Ollama is running";
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

class OllamaClientTest : public testing::Test {
 public:
  OllamaClientTest() = default;
  ~OllamaClientTest() override = default;

  void SetUp() override {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);
    ollama_client_ = std::make_unique<OllamaClient>(shared_url_loader_factory_);
  }

  OllamaClient* ollama_client() { return ollama_client_.get(); }
  network::TestURLLoaderFactory* test_url_loader_factory() {
    return &test_url_loader_factory_;
  }

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<OllamaClient> ollama_client_;
};

TEST_F(OllamaClientTest, CheckConnectionSuccess) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaBaseUrl,
                                         kOllamaSuccessResponse);

  base::RunLoop run_loop;
  ollama_client()->CheckConnection(base::BindLambdaForTesting(
      [&](mojom::OllamaConnectionResultPtr result) {
        EXPECT_TRUE(result->connected);
        EXPECT_EQ("", result->error);
        run_loop.Quit();
      }));

  run_loop.Run();
}

TEST_F(OllamaClientTest, CheckConnectionFailure) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaBaseUrl, "",
                                         net::HTTP_INTERNAL_SERVER_ERROR);

  base::RunLoop run_loop;
  ollama_client()->CheckConnection(base::BindLambdaForTesting(
      [&](mojom::OllamaConnectionResultPtr result) {
        EXPECT_FALSE(result->connected);
        EXPECT_EQ("Ollama is not running at localhost:11434", result->error);
        run_loop.Quit();
      }));

  run_loop.Run();
}

TEST_F(OllamaClientTest, CheckConnectionNoResponse) {
  // Simulate network error with failed response
  test_url_loader_factory()->AddResponse(
      GURL(ai_chat::mojom::kOllamaBaseUrl),
      network::mojom::URLResponseHead::New(), "",
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED));

  base::RunLoop run_loop;
  ollama_client()->CheckConnection(base::BindLambdaForTesting(
      [&](mojom::OllamaConnectionResultPtr result) {
        EXPECT_FALSE(result->connected);
        EXPECT_EQ("Ollama is not running at localhost:11434", result->error);
        run_loop.Quit();
      }));

  run_loop.Run();
}

TEST_F(OllamaClientTest, CheckConnectionWrongResponse) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaBaseUrl,
                                         "Some other response");

  base::RunLoop run_loop;
  ollama_client()->CheckConnection(base::BindLambdaForTesting(
      [&](mojom::OllamaConnectionResultPtr result) {
        EXPECT_FALSE(result->connected);
        EXPECT_EQ("Ollama is not running at localhost:11434", result->error);
        run_loop.Quit();
      }));

  run_loop.Run();
}

TEST_F(OllamaClientTest, FetchModelsSuccess) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         kOllamaModelsResponse);

  base::RunLoop run_loop;
  ollama_client()->FetchModels(
      base::BindLambdaForTesting([&](std::string response_body) {
        EXPECT_EQ(kOllamaModelsResponse, response_body);
        run_loop.Quit();
      }));

  run_loop.Run();
}

TEST_F(OllamaClientTest, FetchModelsNoResponse) {
  // Simulate network error with failed response
  test_url_loader_factory()->AddResponse(
      GURL(ai_chat::mojom::kOllamaApiTagsEndpoint),
      network::mojom::URLResponseHead::New(), "",
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED));

  base::RunLoop run_loop;
  ollama_client()->FetchModels(
      base::BindLambdaForTesting([&](std::string response_body) {
        EXPECT_EQ("", response_body);
        run_loop.Quit();
      }));

  run_loop.Run();
}

TEST_F(OllamaClientTest, FetchModelsEmptyResponse) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaApiTagsEndpoint,
                                         "");

  base::RunLoop run_loop;
  ollama_client()->FetchModels(
      base::BindLambdaForTesting([&](std::string response_body) {
        EXPECT_EQ("", response_body);
        run_loop.Quit();
      }));

  run_loop.Run();
}

}  // namespace ai_chat
