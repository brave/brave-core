// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_fetcher.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/api_request_helper/mock_api_request_helper.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using api_request_helper::MockAPIRequestHelper;
using ResultCallback = api_request_helper::APIRequestHelper::ResultCallback;
using Ticket = api_request_helper::APIRequestHelper::Ticket;
using ::testing::_;

namespace ai_chat {

namespace {

constexpr char kValidModelsJSON[] = R"([
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
  ])";

constexpr char kMissingKeyJSON[] = R"([
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
  ])";

constexpr char kMissingDisplayNameJSON[] = R"([
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
  ])";

constexpr char kMissingOptionsJSON[] = R"([
    {
      "key": "test-model",
      "display_name": "Test Model",
      "is_suggested_model": false,
      "is_near_model": false,
      "capabilities": ["chat", "files"]
    }
  ])";

constexpr char kMissingNameJSON[] = R"([
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
  ])";

constexpr char kMissingAccessJSON[] = R"([
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
  ])";

}  // namespace

class RemoteModelsFetcherTest : public testing::Test {
 public:
  void SetUp() override {
    fetcher_ = std::make_unique<RemoteModelsFetcher>(nullptr);
    auto mock_helper =
        std::make_unique<testing::NiceMock<MockAPIRequestHelper>>(
            TRAFFIC_ANNOTATION_FOR_TESTS, nullptr);
    fetcher_->SetAPIRequestHelperForTesting(std::move(mock_helper));
  }

  void TearDown() override { fetcher_.reset(); }

 protected:
  MockAPIRequestHelper* GetMockAPIRequestHelper() {
    return static_cast<MockAPIRequestHelper*>(
        fetcher_->GetAPIRequestHelperForTesting());
  }

  // Simulates a successful HTTP fetch whose body is |json_response|.
  void SimulateSuccessfulFetch(const std::string& json_response) {
    EXPECT_CALL(*GetMockAPIRequestHelper(), Request(_, _, _, _, _, _, _, _))
        .WillOnce(
            [json_response](
                const std::string& method, const GURL& url,
                const std::string& body, const std::string& content_type,
                ResultCallback result_callback,
                const base::flat_map<std::string, std::string>& headers,
                const api_request_helper::APIRequestOptions& options,
                api_request_helper::APIRequestHelper::ResponseConversionCallback
                    conversion_callback) {
              std::move(result_callback)
                  .Run(api_request_helper::APIRequestResult(
                      net::HTTP_OK, base::test::ParseJson(json_response), {},
                      net::OK, GURL()));
              return Ticket();
            });
  }

  void SimulateHTTPError(int http_code) {
    EXPECT_CALL(*GetMockAPIRequestHelper(), Request(_, _, _, _, _, _, _, _))
        .WillOnce(
            [http_code](
                const std::string& method, const GURL& url,
                const std::string& body, const std::string& content_type,
                ResultCallback result_callback,
                const base::flat_map<std::string, std::string>& headers,
                const api_request_helper::APIRequestOptions& options,
                api_request_helper::APIRequestHelper::ResponseConversionCallback
                    conversion_callback) {
              std::move(result_callback)
                  .Run(api_request_helper::APIRequestResult(
                      http_code, base::Value(), {}, net::OK, GURL()));
              return Ticket();
            });
  }

  void SimulateNetworkError() {
    EXPECT_CALL(*GetMockAPIRequestHelper(), Request(_, _, _, _, _, _, _, _))
        .WillOnce(
            [](const std::string& method, const GURL& url,
               const std::string& body, const std::string& content_type,
               ResultCallback result_callback,
               const base::flat_map<std::string, std::string>& headers,
               const api_request_helper::APIRequestOptions& options,
               api_request_helper::APIRequestHelper::ResponseConversionCallback
                   conversion_callback) {
              std::move(result_callback)
                  .Run(api_request_helper::APIRequestResult(
                      -1, base::Value(), {}, net::ERR_CONNECTION_REFUSED,
                      GURL()));
              return Ticket();
            });
  }

  void ExpectEmptyResult(const std::string& json) {
    SimulateSuccessfulFetch(json);
    base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
    fetcher_->FetchModels(future.GetCallback());
    EXPECT_TRUE(future.Get().empty());
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<RemoteModelsFetcher> fetcher_;
};

TEST_F(RemoteModelsFetcherTest, SuccessfulFetch) {
  SimulateSuccessfulFetch(kValidModelsJSON);

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(future.GetCallback());
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
  fetcher_->FetchModels(future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, NetworkError) {
  SimulateNetworkError();

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, InvalidJSON) {
  // APIRequestHelper leaves value_body() as a default (NONE-type) Value when
  // the response body fails to parse as JSON, while still reporting a 2XX
  // response code.
  EXPECT_CALL(*GetMockAPIRequestHelper(), Request(_, _, _, _, _, _, _, _))
      .WillOnce(
          [](const std::string& method, const GURL& url,
             const std::string& body, const std::string& content_type,
             ResultCallback result_callback,
             const base::flat_map<std::string, std::string>& headers,
             const api_request_helper::APIRequestOptions& options,
             api_request_helper::APIRequestHelper::ResponseConversionCallback
                 conversion_callback) {
            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(
                    net::HTTP_OK, base::Value(), {}, net::OK, GURL()));
            return Ticket();
          });

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, ValidModelsReturnedWhenSomeFail) {
  constexpr char kMixedModelsJSON[] = R"([
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
    ])";

  SimulateSuccessfulFetch(kMixedModelsJSON);

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(future.GetCallback());
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
  ExpectEmptyResult(R"([
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
    ])");
}

TEST_F(RemoteModelsFetcherTest, NoCategoryCapability) {
  ExpectEmptyResult(R"([
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
    ])");
}

TEST_F(RemoteModelsFetcherTest, EmptyResponse) {
  SimulateSuccessfulFetch("[]");

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(RemoteModelsFetcherTest, RejectsUnrecognizedAccessLevel) {
  ExpectEmptyResult(R"([
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
    ])");
}

TEST_F(RemoteModelsFetcherTest, RejectsMissingMaxContentLength) {
  ExpectEmptyResult(R"([
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
    ])");
}

TEST_F(RemoteModelsFetcherTest, RejectsMissingWarningLimit) {
  ExpectEmptyResult(R"([
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
    ])");
}

TEST_F(RemoteModelsFetcherTest, SkipsUnknownCapabilities) {
  constexpr char kUnknownCapabilityJSON[] = R"([
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
    ])";

  SimulateSuccessfulFetch(kUnknownCapabilityJSON);

  base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
  fetcher_->FetchModels(future.GetCallback());
  const auto& fetched_models = future.Get();

  ASSERT_EQ(1u, fetched_models.size());
  ASSERT_EQ(1u, fetched_models[0]->supported_capabilities.size());
  EXPECT_EQ(mojom::ConversationCapability::CHAT,
            fetched_models[0]->supported_capabilities[0]);
}

TEST_F(RemoteModelsFetcherTest, RejectsNegativeMaxContentLength) {
  ExpectEmptyResult(R"([
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
    ])");
}

TEST_F(RemoteModelsFetcherTest, RejectsNegativeWarningLimit) {
  ExpectEmptyResult(R"([
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
    ])");
}

}  // namespace ai_chat
