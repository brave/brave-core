/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ollama/ollama_service.h"

#include <memory>
#include <string>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
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

}  // namespace

class OllamaServiceTest : public testing::Test {
 public:
  OllamaServiceTest() = default;
  ~OllamaServiceTest() override = default;

  void SetUp() override {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &test_url_loader_factory_);
    ollama_client_ =
        std::make_unique<OllamaService>(shared_url_loader_factory_);
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

  base::RunLoop run_loop;
  ollama_client()->IsConnected(base::BindLambdaForTesting([&](bool connected) {
    EXPECT_TRUE(connected);
    run_loop.Quit();
  }));

  run_loop.Run();
}

TEST_F(OllamaServiceTest, ConnectedFailure) {
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaBaseUrl, "",
                                         net::HTTP_INTERNAL_SERVER_ERROR);

  base::RunLoop run_loop;
  ollama_client()->IsConnected(base::BindLambdaForTesting([&](bool connected) {
    EXPECT_FALSE(connected);
    run_loop.Quit();
  }));

  run_loop.Run();
}

TEST_F(OllamaServiceTest, ConnectedNoResponse) {
  // Simulate network error with failed response
  test_url_loader_factory()->AddResponse(
      GURL(ai_chat::mojom::kOllamaBaseUrl),
      network::mojom::URLResponseHead::New(), "",
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED));

  base::RunLoop run_loop;
  ollama_client()->IsConnected(base::BindLambdaForTesting([&](bool connected) {
    EXPECT_FALSE(connected);
    run_loop.Quit();
  }));

  run_loop.Run();
}

TEST_F(OllamaServiceTest, ConnectedWrongResponse) {
  // Any 200 response from Ollama base URL indicates it's running
  test_url_loader_factory()->AddResponse(ai_chat::mojom::kOllamaBaseUrl,
                                         "Some other response");

  base::RunLoop run_loop;
  ollama_client()->IsConnected(base::BindLambdaForTesting([&](bool connected) {
    EXPECT_TRUE(connected);
    run_loop.Quit();
  }));

  run_loop.Run();
}

}  // namespace ai_chat
