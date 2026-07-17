// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_fetcher.h"

#include <memory>
#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
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
      "is_suggested_model": true,
      "is_near_model": false,
      "capabilities": ["chat", "files"],
      "options": {
        "name": "test-model-1-api",
        "display_maker": "Test Provider",
        "description": "A basic test model",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    },
    {
      "key": "test-model-2",
      "display_name": "Test Model 2",
      "is_suggested_model": false,
      "is_near_model": false,
      "capabilities": ["chat", "files", "content_agent"],
      "options": {
        "name": "test-model-2-api",
        "display_maker": "Test Provider",
        "description": "A premium test model",
        "access": "premium",
        "max_associated_content_length": 150000,
        "long_conversation_warning_character_limit": 300000
      }
    },
    {
      "key": "test-model-3",
      "display_name": "Test Model 3",
      "is_suggested_model": false,
      "is_near_model": false,
      "capabilities": ["summary", "files"],
      "options": {
        "name": "test-model-3-api",
        "display_maker": "Test Provider",
        "description": "A summary model",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    }
  ]
})";

constexpr char kInvalidJSON[] = "{ invalid json";

constexpr char kMissingKeyJSON[] = R"({
  "models": [
    {
      "display_name": "Test Model",
      "is_suggested_model": false,
      "is_near_model": false,
      "capabilities": ["chat", "files"],
      "options": {
        "name": "test-model-api",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    }
  ]
})";

constexpr char kMissingDisplayNameJSON[] = R"({
  "models": [
    {
      "key": "test-model",
      "is_suggested_model": false,
      "is_near_model": false,
      "capabilities": ["chat", "files"],
      "options": {
        "name": "test-model-api",
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    }
  ]
})";

constexpr char kMissingOptionsJSON[] = R"({
  "models": [
    {
      "key": "test-model",
      "display_name": "Test Model",
      "is_suggested_model": false,
      "is_near_model": false,
      "capabilities": ["chat", "files"]
    }
  ]
})";

constexpr char kMissingNameJSON[] = R"({
  "models": [
    {
      "key": "test-model",
      "display_name": "Test Model",
      "is_suggested_model": false,
      "is_near_model": false,
      "capabilities": ["chat", "files"],
      "options": {
        "access": "basic",
        "max_associated_content_length": 100000,
        "long_conversation_warning_character_limit": 200000
      }
    }
  ]
})";

constexpr char kMissingAccessJSON[] = R"({
  "models": [
    {
      "key": "test-model",
      "display_name": "Test Model",
      "is_suggested_model": false,
      "is_near_model": false,
      "capabilities": ["chat", "files"],
      "options": {
        "name": "test-model-api",
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
          } else {
            ADD_FAILURE() << "Unexpected request: " << request.url.spec();
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
          } else {
            ADD_FAILURE() << "Unexpected request: " << request.url.spec();
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
          } else {
            ADD_FAILURE() << "Unexpected request: " << request.url.spec();
          }
        }));
  }

  void ExpectEmptyResult(const std::string& json) {
    SimulateSuccessfulFetch(json);
    base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
    fetcher_->FetchModels(kTestEndpoint, future.GetCallback());
    EXPECT_TRUE(future.Get().empty());
  }

  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<RemoteModelsFetcher> fetcher_;
};

TEST_F(RemoteModelsFetcherTest, SuccessfulFetch) {
  SimulateSuccessfulFetch(kValidModelsJSON);

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(kTestEndpoint, future.GetCallback());
  const auto& fetched_models = future.Get();

  ASSERT_EQ(3u, fetched_models.size());

  EXPECT_EQ("test-model-1", fetched_models[0]->key);
  EXPECT_EQ("Test Model 1", fetched_models[0]->display_name);
  EXPECT_TRUE(fetched_models[0]->is_suggested_model);
  EXPECT_FALSE(fetched_models[0]->is_near_model);
  ASSERT_TRUE(fetched_models[0]->options->is_leo_model_options());
  auto& opts1 = fetched_models[0]->options->get_leo_model_options();
  EXPECT_EQ("test-model-1-api", opts1->name);
  EXPECT_EQ("Test Provider", opts1->display_maker);
  EXPECT_EQ("A basic test model", opts1->description);
  EXPECT_EQ(mojom::ModelCategory::CHAT, opts1->category);
  EXPECT_EQ(mojom::ModelAccess::BASIC, opts1->access);
  EXPECT_EQ(100000u, opts1->max_associated_content_length);
  EXPECT_EQ(200000u, opts1->long_conversation_warning_character_limit);
  ASSERT_EQ(2u, fetched_models[0]->supported_capabilities.size());
  EXPECT_EQ(mojom::ConversationCapability::CHAT,
            fetched_models[0]->supported_capabilities[0]);
  EXPECT_EQ(mojom::ConversationCapability::FILES,
            fetched_models[0]->supported_capabilities[1]);

  EXPECT_EQ("test-model-2", fetched_models[1]->key);
  EXPECT_EQ("Test Model 2", fetched_models[1]->display_name);
  EXPECT_FALSE(fetched_models[1]->is_suggested_model);
  EXPECT_FALSE(fetched_models[1]->is_near_model);
  ASSERT_TRUE(fetched_models[1]->options->is_leo_model_options());
  auto& opts2 = fetched_models[1]->options->get_leo_model_options();
  EXPECT_EQ("test-model-2-api", opts2->name);
  EXPECT_EQ("Test Provider", opts2->display_maker);
  EXPECT_EQ("A premium test model", opts2->description);
  EXPECT_EQ(mojom::ModelCategory::CHAT, opts2->category);
  EXPECT_EQ(mojom::ModelAccess::PREMIUM, opts2->access);
  EXPECT_EQ(150000u, opts2->max_associated_content_length);
  EXPECT_EQ(300000u, opts2->long_conversation_warning_character_limit);
  ASSERT_EQ(3u, fetched_models[1]->supported_capabilities.size());
  EXPECT_EQ(mojom::ConversationCapability::CHAT,
            fetched_models[1]->supported_capabilities[0]);
  EXPECT_EQ(mojom::ConversationCapability::FILES,
            fetched_models[1]->supported_capabilities[1]);
  EXPECT_EQ(mojom::ConversationCapability::CONTENT_AGENT,
            fetched_models[1]->supported_capabilities[2]);

  EXPECT_EQ("test-model-3", fetched_models[2]->key);
  EXPECT_EQ("Test Model 3", fetched_models[2]->display_name);
  ASSERT_TRUE(fetched_models[2]->options->is_leo_model_options());
  auto& opts3 = fetched_models[2]->options->get_leo_model_options();
  EXPECT_EQ(mojom::ModelCategory::SUMMARY, opts3->category);
  EXPECT_EQ(mojom::ModelAccess::BASIC, opts3->access);
  ASSERT_EQ(2u, fetched_models[2]->supported_capabilities.size());
  EXPECT_EQ(mojom::ConversationCapability::SUMMARY,
            fetched_models[2]->supported_capabilities[0]);
  EXPECT_EQ(mojom::ConversationCapability::FILES,
            fetched_models[2]->supported_capabilities[1]);
}

TEST_F(RemoteModelsFetcherTest, HTTPError500) {
  SimulateHTTPError(500);

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(kTestEndpoint, future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, NetworkError) {
  SimulateNetworkError();

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(kTestEndpoint, future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, InvalidJSON) {
  SimulateSuccessfulFetch(kInvalidJSON);

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(kTestEndpoint, future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, ValidModelsReturnedWhenSomeFail) {
  constexpr char kMixedModelsJSON[] = R"({
    "models": [
      {
        "key": "valid-model",
        "display_name": "Valid Model",
        "is_suggested_model": false,
        "is_near_model": false,
        "capabilities": ["chat", "files"],
        "options": {
          "name": "valid-model-api",
          "display_maker": "Test Provider",
          "access": "basic",
          "max_associated_content_length": 100000,
          "long_conversation_warning_character_limit": 200000
        }
      },
      {
        "display_name": "Invalid Model - Missing Key",
        "is_suggested_model": false,
        "is_near_model": false,
        "capabilities": ["chat", "files"],
        "options": {
          "name": "invalid-model-api",
          "access": "basic",
          "max_associated_content_length": 100000,
          "long_conversation_warning_character_limit": 200000
        }
      }
    ]
  })";

  SimulateSuccessfulFetch(kMixedModelsJSON);

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(kTestEndpoint, future.GetCallback());
  const auto& fetched_models = future.Get();

  ASSERT_EQ(1u, fetched_models.size());
  EXPECT_EQ("valid-model", fetched_models[0]->key);
}

TEST_F(RemoteModelsFetcherTest, RequiredFieldsRejected) {
  const struct {
    const char* name;
    const char* json;
  } kCases[] = {
      {"MissingKey", kMissingKeyJSON},
      {"MissingDisplayName", kMissingDisplayNameJSON},
      {"MissingOptions", kMissingOptionsJSON},
      {"MissingName", kMissingNameJSON},
      {"MissingAccess", kMissingAccessJSON},
  };

  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.name);
    ExpectEmptyResult(test_case.json);
  }
}

TEST_F(RemoteModelsFetcherTest, MissingCapabilities) {
  ExpectEmptyResult(R"({
    "models": [
      {
        "key": "test-model",
        "display_name": "Test Model",
        "is_suggested_model": false,
        "is_near_model": false,
        "options": {
          "name": "test-model-api",
          "access": "basic",
          "max_associated_content_length": 100000,
          "long_conversation_warning_character_limit": 200000
        }
      }
    ]
  })");
}

TEST_F(RemoteModelsFetcherTest, NoCategoryCapability) {
  ExpectEmptyResult(R"({
    "models": [
      {
        "key": "test-model",
        "display_name": "Test Model",
        "is_suggested_model": false,
        "is_near_model": false,
        "capabilities": ["files"],
        "options": {
          "name": "test-model-api",
          "access": "basic",
          "max_associated_content_length": 100000,
          "long_conversation_warning_character_limit": 200000
        }
      }
    ]
  })");
}

TEST_F(RemoteModelsFetcherTest, RejectsHTTPEndpoint) {
  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels("http://example.com/models", future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, RejectsHTTPForLocalhost) {
  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels("http://localhost:8080/models", future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, RejectsInvalidURL) {
  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels("not-a-valid-url", future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, EmptyResponse) {
  SimulateSuccessfulFetch(R"({"models": []})");

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(kTestEndpoint, future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, RejectsUnrecognizedAccessLevel) {
  ExpectEmptyResult(R"({
    "models": [
      {
        "key": "unknown-access-model",
        "display_name": "Unknown Access Model",
        "is_suggested_model": false,
        "is_near_model": false,
        "capabilities": ["chat", "files"],
        "options": {
          "name": "unknown-access-api",
          "display_maker": "Test Provider",
          "access": "enterprise",
          "max_associated_content_length": 100000,
          "long_conversation_warning_character_limit": 200000
        }
      }
    ]
  })");
}

TEST_F(RemoteModelsFetcherTest, RejectsMissingMaxContentLength) {
  ExpectEmptyResult(R"({
    "models": [
      {
        "key": "basic-model",
        "display_name": "Basic Model",
        "is_suggested_model": false,
        "is_near_model": false,
        "capabilities": ["chat", "files"],
        "options": {
          "name": "basic-model-api",
          "display_maker": "Test Provider",
          "access": "basic",
          "long_conversation_warning_character_limit": 200000
        }
      }
    ]
  })");
}

TEST_F(RemoteModelsFetcherTest, RejectsMissingWarningLimit) {
  ExpectEmptyResult(R"({
    "models": [
      {
        "key": "basic-model",
        "display_name": "Basic Model",
        "is_suggested_model": false,
        "is_near_model": false,
        "capabilities": ["chat", "files"],
        "options": {
          "name": "basic-model-api",
          "display_maker": "Test Provider",
          "access": "basic",
          "max_associated_content_length": 100000
        }
      }
    ]
  })");
}

TEST_F(RemoteModelsFetcherTest, SkipsUnknownCapabilities) {
  constexpr char kUnknownCapabilityJSON[] = R"({
    "models": [
      {
        "key": "test-model",
        "display_name": "Test Model",
        "is_suggested_model": false,
        "is_near_model": false,
        "capabilities": ["chat", "unknown_capability"],
        "options": {
          "name": "test-model-api",
          "display_maker": "Test Provider",
          "access": "basic",
          "max_associated_content_length": 100000,
          "long_conversation_warning_character_limit": 200000
        }
      }
    ]
  })";

  SimulateSuccessfulFetch(kUnknownCapabilityJSON);

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(kTestEndpoint, future.GetCallback());
  const auto& fetched_models = future.Get();

  ASSERT_EQ(1u, fetched_models.size());
  ASSERT_EQ(1u, fetched_models[0]->supported_capabilities.size());
  EXPECT_EQ(mojom::ConversationCapability::CHAT,
            fetched_models[0]->supported_capabilities[0]);
}

TEST_F(RemoteModelsFetcherTest, RejectsNegativeMaxContentLength) {
  ExpectEmptyResult(R"({
    "models": [
      {
        "key": "bad-model",
        "display_name": "Bad Model",
        "is_suggested_model": false,
        "is_near_model": false,
        "capabilities": ["chat", "files"],
        "options": {
          "name": "bad-model-api",
          "display_maker": "Test Provider",
          "access": "basic",
          "max_associated_content_length": -1,
          "long_conversation_warning_character_limit": 200000
        }
      }
    ]
  })");
}

TEST_F(RemoteModelsFetcherTest, RejectsNegativeWarningLimit) {
  ExpectEmptyResult(R"({
    "models": [
      {
        "key": "bad-model",
        "display_name": "Bad Model",
        "is_suggested_model": false,
        "is_near_model": false,
        "capabilities": ["chat", "files"],
        "options": {
          "name": "bad-model-api",
          "display_maker": "Test Provider",
          "access": "basic",
          "max_associated_content_length": 100000,
          "long_conversation_warning_character_limit": -1
        }
      }
    ]
  })");
}

}  // namespace ai_chat
