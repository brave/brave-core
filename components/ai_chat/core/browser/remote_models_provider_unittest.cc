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
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/remote_models_serialization.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kTestEndpoint[] = "https://models.example.com/models.json";
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
  base::DictValue root;
  root.Set(kModelsKey, SerializeModels(models));
  return base::WriteJson(root).value_or("");
}

}  // namespace

class RemoteModelsProviderTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    prefs::RegisterProfilePrefs(pref_service_.registry());
  }

  std::unique_ptr<RemoteModelsProvider> MakeProvider() {
    return std::make_unique<RemoteModelsProvider>(
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_),
        &pref_service_, temp_dir_.GetPath().AppendASCII("remote_models.json"),
        kTestTTL, kTestEndpoint);
  }

  std::vector<mojom::ModelPtr> GetModels(RemoteModelsProvider& provider) {
    base::test::TestFuture<std::vector<mojom::ModelPtr>> future;
    provider.GetModels(future.GetCallback());
    return future.Take();
  }

  void RespondWithModels(const std::vector<mojom::ModelPtr>& models) {
    url_loader_factory_.AddResponse(kTestEndpoint, MakeModelsResponse(models));
  }

  void RespondWithError() {
    url_loader_factory_.AddResponse(kTestEndpoint, "",
                                    net::HTTP_INTERNAL_SERVER_ERROR);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple pref_service_;
  network::TestURLLoaderFactory url_loader_factory_;
};

TEST_F(RemoteModelsProviderTest, FetchesWhenCacheEmpty) {
  std::vector<mojom::ModelPtr> server_models;
  server_models.push_back(MakeTestModel("model-a"));
  RespondWithModels(server_models);

  auto provider = MakeProvider();
  auto result = GetModels(*provider);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0]->key, "model-a");
}

TEST_F(RemoteModelsProviderTest, ReturnsCachedModelsWithoutFetch) {
  // First call fetches and caches.
  std::vector<mojom::ModelPtr> server_models;
  server_models.push_back(MakeTestModel("model-a"));
  RespondWithModels(server_models);

  auto provider = MakeProvider();
  GetModels(*provider);

  // Wait for OnWriteComplete to set the pref timestamp before the second call
  // checks the cache.
  ASSERT_TRUE(base::test::RunUntil([&] {
    return !pref_service_.GetTime(prefs::kRemoteModelsCachedAt).is_null();
  }));

  // Second call within TTL — no new network request should be needed.
  // Clear responses so any network request would return nothing.
  url_loader_factory_.ClearResponses();
  auto result = GetModels(*provider);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0]->key, "model-a");
}

TEST_F(RemoteModelsProviderTest, ExpiredCacheTriggersRefetch) {
  std::vector<mojom::ModelPtr> first;
  first.push_back(MakeTestModel("old-model"));
  RespondWithModels(first);

  auto provider = MakeProvider();
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
  RespondWithModels(second);

  auto result = GetModels(*provider);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0]->key, "new-model");
}

TEST_F(RemoteModelsProviderTest, FetchFailureReturnsEmptyVector) {
  RespondWithError();
  auto provider = MakeProvider();
  auto result = GetModels(*provider);
  EXPECT_TRUE(result.empty());
}

}  // namespace ai_chat
