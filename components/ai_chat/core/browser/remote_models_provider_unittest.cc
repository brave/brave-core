// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_provider.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/json/json_writer.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/remote_models_serialization.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/api_request_helper/mock_api_request_helper.h"
#include "components/prefs/testing_pref_service.h"
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

constexpr base::TimeDelta kTestTTL = base::Hours(24);

mojom::ModelPtr MakeTestModel(const std::string& key) {
  auto leo_opts = mojom::LeoModelOptions::New();
  leo_opts->name = key + "-model";
  leo_opts->display_maker = "Test Corp";
  leo_opts->description = "A test model";
  leo_opts->category = mojom::ModelCategory::CHAT;
  leo_opts->access = mojom::ModelAccess::BASIC;
  leo_opts->max_associated_content_length = 100000;
  leo_opts->long_conversation_warning_character_limit = 200000;

  auto model = mojom::Model::New();
  model->key = key;
  model->display_name = key + " Display";
  model->is_suggested_model = false;
  model->is_near_model = false;
  model->supported_capabilities = {mojom::ConversationCapability::CHAT};
  model->options = mojom::ModelOptions::NewLeoModelOptions(std::move(leo_opts));
  return model;
}

// Builds a server-format JSON response containing |models|.
std::string MakeModelsResponse(const std::vector<mojom::ModelPtr>& models) {
  return base::WriteJson(SerializeModels(models)).value_or("");
}

}  // namespace

class RemoteModelsProviderTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    prefs::RegisterProfilePrefs(pref_service_.registry());
  }

  std::unique_ptr<RemoteModelsProvider> MakeProvider() {
    auto provider = std::make_unique<RemoteModelsProvider>(
        nullptr, &pref_service_, temp_dir_.GetPath());
    provider->GetFetcherForTesting().SetAPIRequestHelperForTesting(
        std::make_unique<testing::NiceMock<MockAPIRequestHelper>>(
            TRAFFIC_ANNOTATION_FOR_TESTS, nullptr));
    return provider;
  }

  MockAPIRequestHelper* GetMockAPIRequestHelper(
      RemoteModelsProvider& provider) {
    return static_cast<MockAPIRequestHelper*>(
        provider.GetFetcherForTesting().GetAPIRequestHelperForTesting());
  }

  std::vector<mojom::ModelPtr> GetModels(RemoteModelsProvider& provider) {
    base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
    provider.GetModels(future.GetCallback());
    return future.Take();
  }

  void RespondWithModels(RemoteModelsProvider& provider,
                         const std::vector<mojom::ModelPtr>& models) {
    std::string response = MakeModelsResponse(models);
    EXPECT_CALL(*GetMockAPIRequestHelper(provider),
                Request(_, _, _, _, _, _, _, _))
        .WillOnce(
            [response](
                const std::string& method, const GURL& url,
                const std::string& body, const std::string& content_type,
                ResultCallback result_callback,
                const base::flat_map<std::string, std::string>& headers,
                const api_request_helper::APIRequestOptions& options,
                api_request_helper::APIRequestHelper::ResponseConversionCallback
                    conversion_callback) {
              std::move(result_callback)
                  .Run(api_request_helper::APIRequestResult(
                      net::HTTP_OK, base::test::ParseJson(response), {},
                      net::OK, GURL()));
              return Ticket();
            });
  }

  void RespondWithError(RemoteModelsProvider& provider) {
    EXPECT_CALL(*GetMockAPIRequestHelper(provider),
                Request(_, _, _, _, _, _, _, _))
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
                      net::HTTP_INTERNAL_SERVER_ERROR, base::Value(), {},
                      net::OK, GURL()));
              return Ticket();
            });
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple pref_service_;
};

TEST_F(RemoteModelsProviderTest, FetchesWhenCacheEmpty) {
  auto provider = MakeProvider();

  std::vector<mojom::ModelPtr> server_models;
  server_models.push_back(MakeTestModel("model-a"));
  RespondWithModels(*provider, server_models);

  auto result = GetModels(*provider);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0]->key, "model-a");
}

TEST_F(RemoteModelsProviderTest, ReturnsCachedModelsWithoutFetch) {
  auto provider = MakeProvider();

  // First call fetches and caches.
  std::vector<mojom::ModelPtr> server_models;
  server_models.push_back(MakeTestModel("model-a"));
  RespondWithModels(*provider, server_models);
  GetModels(*provider);

  // Wait for OnWriteComplete to set the pref timestamp before the second call
  // checks the cache.
  ASSERT_TRUE(base::test::RunUntil([&] {
    return !pref_service_.GetTime(prefs::kRemoteModelsCachedAt).is_null();
  }));

  // Second call within TTL — no new network request should be needed.
  auto result = GetModels(*provider);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0]->key, "model-a");
}

TEST_F(RemoteModelsProviderTest, ExpiredCacheTriggersRefetch) {
  auto provider = MakeProvider();

  std::vector<mojom::ModelPtr> first;
  first.push_back(MakeTestModel("old-model"));
  RespondWithModels(*provider, first);
  GetModels(*provider);

  // Wait for OnWriteComplete to set the pref timestamp before advancing the
  // clock past TTL.
  ASSERT_TRUE(base::test::RunUntil([&] {
    return !pref_service_.GetTime(prefs::kRemoteModelsCachedAt).is_null();
  }));

  // Advance past TTL, then respond with new models.
  task_environment_.AdvanceClock(kTestTTL + base::Minutes(1));

  std::vector<mojom::ModelPtr> second;
  second.push_back(MakeTestModel("new-model"));
  RespondWithModels(*provider, second);

  auto result = GetModels(*provider);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0]->key, "new-model");
}

TEST_F(RemoteModelsProviderTest, FetchFailureReturnsEmptyVector) {
  auto provider = MakeProvider();
  RespondWithError(*provider);
  auto result = GetModels(*provider);
  EXPECT_TRUE(result.empty());
}

}  // namespace ai_chat
