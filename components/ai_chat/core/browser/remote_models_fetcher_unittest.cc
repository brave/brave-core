// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_fetcher.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kTestEndpoint[] = "https://example.com/models";

constexpr char kValidModelsJSON[] = R"({
  "models": [
    {
      "key": "test-model-1",
      "display_name": "Test Model 1",
      "vision_support": true,
      "supports_tools": false,
      "is_suggested_model": true,
      "is_near_model": false,
      "options": {
        "type": "leo",
        "name": "test-model-1-api",
        "display_maker": "Test Provider",
        "description": "A basic test model",
        "category": "chat",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    },
    {
      "key": "test-model-2",
      "display_name": "Test Model 2",
      "vision_support": false,
      "supports_tools": true,
      "is_suggested_model": false,
      "is_near_model": false,
      "options": {
        "type": "leo",
        "name": "test-model-2-api",
        "display_maker": "Test Provider",
        "description": "A premium test model",
        "category": "chat",
        "access": "premium",
        "max_associated_content_length": 150000,
        "long_conversation_warning_character_limit": 300000
      }
    }
  ]
})";

constexpr char kInvalidJSON[] = "{ invalid json";

constexpr char kMissingKeyJSON[] = R"({
  "models": [
    {
      "display_name": "Missing Key Model",
      "vision_support": true,
      "supports_tools": false,
      "is_suggested_model": true,
      "is_near_model": false,
      "options": {
        "type": "leo",
        "name": "missing-key-model",
        "category": "chat",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    }
  ]
})";

constexpr char kInvalidTypeJSON[] = R"({
  "models": [
    {
      "key": "invalid-type-model",
      "display_name": "Invalid Type Model",
      "vision_support": true,
      "supports_tools": false,
      "is_suggested_model": true,
      "is_near_model": false,
      "options": {
        "type": "custom",
        "name": "invalid-type-model",
        "category": "chat",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    }
  ]
})";

}  // namespace

class RemoteModelsFetcherTest : public testing::Test {
 public:
  RemoteModelsFetcherTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {}

  void SetUp() override {
    fetcher_ =
        std::make_unique<RemoteModelsFetcher>(shared_url_loader_factory_);
  }

  void TearDown() override { fetcher_.reset(); }

 protected:
  void SimulateSuccessfulFetch(const std::string& json_response,
                               const std::string& base_url = kTestEndpoint) {
    test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this, json_response,
         base_url](const network::ResourceRequest& request) {
          if (base::StartsWith(request.url.spec(), base_url)) {
            test_url_loader_factory_.AddResponse(request.url.spec(),
                                                 json_response);
          }
        }));
  }

  void SimulateHTTPError(int http_code,
                         const std::string& base_url = kTestEndpoint) {
    test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this, http_code, base_url](const network::ResourceRequest& request) {
          if (base::StartsWith(request.url.spec(), base_url)) {
            test_url_loader_factory_.AddResponse(
                request.url.spec(), "",
                static_cast<net::HttpStatusCode>(http_code));
          }
        }));
  }

  void SimulateNetworkError(const std::string& base_url = kTestEndpoint) {
    test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [this, base_url](const network::ResourceRequest& request) {
          if (base::StartsWith(request.url.spec(), base_url)) {
            test_url_loader_factory_.AddResponse(
                request.url, network::mojom::URLResponseHead::New(), "",
                network::URLLoaderCompletionStatus(
                    net::ERR_CONNECTION_REFUSED));
          }
        }));
  }

  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<RemoteModelsFetcher> fetcher_;
};

TEST_F(RemoteModelsFetcherTest, SuccessfulFetch) {
  SimulateSuccessfulFetch(kValidModelsJSON);

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  ASSERT_EQ(2u, fetched_models.size());

  EXPECT_EQ("test-model-1", fetched_models[0]->key);
  EXPECT_EQ("Test Model 1", fetched_models[0]->display_name);
  EXPECT_TRUE(fetched_models[0]->vision_support);
  EXPECT_FALSE(fetched_models[0]->supports_tools);

  EXPECT_EQ("test-model-2", fetched_models[1]->key);
  EXPECT_EQ("Test Model 2", fetched_models[1]->display_name);
  EXPECT_FALSE(fetched_models[1]->vision_support);
  EXPECT_TRUE(fetched_models[1]->supports_tools);
}

TEST_F(RemoteModelsFetcherTest, HTTPError404) {
  SimulateHTTPError(404);

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, HTTPError500) {
  SimulateHTTPError(500);

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, NetworkError) {
  SimulateNetworkError();

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, InvalidJSON) {
  SimulateSuccessfulFetch(kInvalidJSON);

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, MissingRequiredField) {
  SimulateSuccessfulFetch(kMissingKeyJSON);

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, InvalidModelType) {
  SimulateSuccessfulFetch(kInvalidTypeJSON);

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, RejectsHTTPEndpoint) {
  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      "http://example.com/models",
      base::BindOnce(
          [](bool* called, std::vector<mojom::ModelPtr>* models,
             std::vector<mojom::ModelPtr> result) {
            *called = true;
            *models = std::move(result);
          },
          &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, RejectsHTTPForLocalhost) {
  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      "http://localhost:8080/models",
      base::BindOnce(
          [](bool* called, std::vector<mojom::ModelPtr>* models,
             std::vector<mojom::ModelPtr> result) {
            *called = true;
            *models = std::move(result);
          },
          &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, RejectsInvalidURL) {
  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      "not-a-valid-url",
      base::BindOnce(
          [](bool* called, std::vector<mojom::ModelPtr>* models,
             std::vector<mojom::ModelPtr> result) {
            *called = true;
            *models = std::move(result);
          },
          &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, EmptyResponse) {
  SimulateSuccessfulFetch(R"({"models": []})");

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, RejectsUnrecognizedAccessLevel) {
  constexpr char kUnknownAccessJSON[] = R"({
    "models": [
      {
        "key": "unknown-access-model",
        "display_name": "Unknown Access Model",
        "vision_support": false,
        "supports_tools": false,
        "is_suggested_model": false,
        "is_near_model": false,
        "options": {
          "type": "leo",
          "name": "unknown-access-api",
          "display_maker": "Test Provider",
          "category": "chat",
          "access": "enterprise",
          "max_associated_content_length": 100000,
          "long_conversation_warning_character_limit": 200000
        }
      }
    ]
  })";

  SimulateSuccessfulFetch(kUnknownAccessJSON);

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  EXPECT_TRUE(fetched_models.empty());
}

TEST_F(RemoteModelsFetcherTest, SummaryCategory) {
  constexpr char kSummaryModelJSON[] = R"({
    "models": [
      {
        "key": "summary-model",
        "display_name": "Summary Model",
        "vision_support": false,
        "supports_tools": false,
        "is_suggested_model": false,
        "is_near_model": false,
        "options": {
          "type": "leo",
          "name": "summary-model-api",
          "display_maker": "Test Provider",
          "description": "A summary model",
          "category": "summary",
          "access": "basic",
          "max_associated_content_length": 100000,
          "long_conversation_warning_character_limit": 200000
        }
      }
    ]
  })";

  SimulateSuccessfulFetch(kSummaryModelJSON);

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  ASSERT_EQ(1u, fetched_models.size());
  ASSERT_TRUE(fetched_models[0]->options->is_leo_model_options());
  EXPECT_EQ(fetched_models[0]->options->get_leo_model_options()->category,
            mojom::ModelCategory::SUMMARY);
}

TEST_F(RemoteModelsFetcherTest, MissingNumericFieldsGetTierDefaults) {
  constexpr char kMissingNumericFieldsJSON[] = R"({
    "models": [
      {
        "key": "basic-model",
        "display_name": "Basic Model",
        "vision_support": false,
        "supports_tools": false,
        "is_suggested_model": false,
        "is_near_model": false,
        "options": {
          "type": "leo",
          "name": "basic-model-api",
          "display_maker": "Test Provider",
          "category": "chat",
          "access": "basic"
        }
      },
      {
        "key": "premium-model",
        "display_name": "Premium Model",
        "vision_support": false,
        "supports_tools": false,
        "is_suggested_model": false,
        "is_near_model": false,
        "options": {
          "type": "leo",
          "name": "premium-model-api",
          "display_maker": "Test Provider",
          "category": "chat",
          "access": "premium"
        }
      }
    ]
  })";

  SimulateSuccessfulFetch(kMissingNumericFieldsJSON);

  bool callback_called = false;
  std::vector<mojom::ModelPtr> fetched_models;

  fetcher_->FetchModels(
      kTestEndpoint, base::BindOnce(
                         [](bool* called, std::vector<mojom::ModelPtr>* models,
                            std::vector<mojom::ModelPtr> result) {
                           *called = true;
                           *models = std::move(result);
                         },
                         &callback_called, &fetched_models));

  EXPECT_TRUE(base::test::RunUntil([&]() { return callback_called; }));
  ASSERT_EQ(2u, fetched_models.size());

  ASSERT_TRUE(fetched_models[0]->options->is_leo_model_options());
  auto& basic_opts = fetched_models[0]->options->get_leo_model_options();
  EXPECT_EQ(basic_opts->max_associated_content_length, 32000u);
  EXPECT_EQ(basic_opts->long_conversation_warning_character_limit, 51200u);

  ASSERT_TRUE(fetched_models[1]->options->is_leo_model_options());
  auto& premium_opts = fetched_models[1]->options->get_leo_model_options();
  EXPECT_EQ(premium_opts->max_associated_content_length, 90000u);
  EXPECT_EQ(premium_opts->long_conversation_warning_character_limit, 160000u);
}

}  // namespace ai_chat
