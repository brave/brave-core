// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_disk_cache.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
// Matches the default Griffin param value (24 hours).
constexpr base::TimeDelta kDefaultTTL = base::Hours(24);
}  // namespace

namespace ai_chat {

namespace {

mojom::ModelPtr MakeTestModel(const std::string& key, const std::string& name) {
  auto leo_opts = mojom::LeoModelOptions::New();
  leo_opts->name = name;
  leo_opts->display_maker = "Test Corp";
  leo_opts->description = "A test model";
  leo_opts->category = mojom::ModelCategory::CHAT;
  leo_opts->access = mojom::ModelAccess::BASIC_AND_PREMIUM;
  leo_opts->max_associated_content_length = 32000;
  leo_opts->long_conversation_warning_character_limit = 51200;

  auto model = mojom::Model::New();
  model->key = key;
  model->display_name = name + " Display";
  model->is_suggested_model = false;
  model->is_near_model = false;
  model->supported_capabilities = {mojom::ConversationCapability::CHAT};
  model->options = mojom::ModelOptions::NewLeoModelOptions(std::move(leo_opts));
  return model;
}

}  // namespace

class RemoteModelsDiskCacheTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    prefs::RegisterProfilePrefs(pref_service_.registry());
  }

  base::FilePath CachePath() const {
    return temp_dir_.GetPath().AppendASCII("ai_chat_remote_models.json");
  }

  RemoteModelsDiskCache MakeCache() {
    return RemoteModelsDiskCache(CachePath(), kDefaultTTL, &pref_service_);
  }

  // Runs Load() and blocks until the callback fires.
  std::optional<std::vector<mojom::ModelPtr>> RunLoad(
      RemoteModelsDiskCache& cache) {
    base::test::TestFuture<std::optional<std::vector<mojom::ModelPtr>>> future;
    cache.Load(future.GetCallback());
    return future.Take();
  }

  void SaveAndWait(RemoteModelsDiskCache& cache,
                   std::vector<mojom::ModelPtr> models) {
    base::test::TestFuture<void> future;
    cache.Save(std::move(models), future.GetCallback());
    ASSERT_TRUE(future.Wait());
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple pref_service_;
};

TEST_F(RemoteModelsDiskCacheTest, LoadMissingFile) {
  auto cache = MakeCache();
  EXPECT_FALSE(RunLoad(cache).has_value());
}

TEST_F(RemoteModelsDiskCacheTest, LoadCorruptJSON) {
  ASSERT_TRUE(base::WriteFile(CachePath(), "not valid json {{"));
  // Write a valid timestamp so the TTL check passes and the file is read.
  pref_service_.SetTime(prefs::kRemoteModelsCachedAt, base::Time::Now());
  auto cache = MakeCache();
  EXPECT_FALSE(RunLoad(cache).has_value());
}

TEST_F(RemoteModelsDiskCacheTest, LoadMissingCachedAtPref) {
  // File exists but no timestamp in prefs — treated as expired.
  ASSERT_TRUE(base::WriteFile(CachePath(), R"({"models": []})"));
  auto cache = MakeCache();
  EXPECT_FALSE(RunLoad(cache).has_value());
}

TEST_F(RemoteModelsDiskCacheTest, SaveAndLoad) {
  auto cache = MakeCache();

  std::vector<mojom::ModelPtr> models_to_save;
  models_to_save.push_back(MakeTestModel("model-key-1", "model-name-1"));
  models_to_save.push_back(MakeTestModel("model-key-2", "model-name-2"));
  SaveAndWait(cache, std::move(models_to_save));

  auto result = RunLoad(cache);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 2u);
  EXPECT_EQ((*result)[0]->key, "model-key-1");
  EXPECT_EQ((*result)[1]->key, "model-key-2");
}

TEST_F(RemoteModelsDiskCacheTest, CacheWithinTTLIsValid) {
  auto cache = MakeCache();

  std::vector<mojom::ModelPtr> models;
  models.push_back(MakeTestModel("model-key-1", "model-name-1"));
  SaveAndWait(cache, std::move(models));

  // Advance to one minute before expiry.
  task_environment_.AdvanceClock(kDefaultTTL - base::Minutes(1));

  auto result = RunLoad(cache);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 1u);
}

TEST_F(RemoteModelsDiskCacheTest, ExpiredCacheReturnsNullopt) {
  auto cache = MakeCache();

  std::vector<mojom::ModelPtr> models;
  models.push_back(MakeTestModel("model-key-1", "model-name-1"));
  SaveAndWait(cache, std::move(models));

  // Advance past the TTL.
  task_environment_.AdvanceClock(kDefaultTTL + base::Minutes(1));

  EXPECT_FALSE(RunLoad(cache).has_value());
}

TEST_F(RemoteModelsDiskCacheTest, RoundTripPreservesModelFields) {
  auto cache = MakeCache();

  auto model = MakeTestModel("chat/claude-3-haiku", "claude-haiku-20240307");
  model->is_suggested_model = true;
  model->is_near_model = false;
  model->supported_capabilities = {mojom::ConversationCapability::CHAT,
                                   mojom::ConversationCapability::FILES};
  {
    auto& leo = model->options->get_leo_model_options();
    leo->display_maker = "Anthropic";
    leo->description = "Fast and capable";
    leo->access = mojom::ModelAccess::PREMIUM;
    leo->max_associated_content_length = 90000;
    leo->long_conversation_warning_character_limit = 160000;
  }

  std::vector<mojom::ModelPtr> save;
  save.push_back(std::move(model));
  SaveAndWait(cache, std::move(save));

  auto result = RunLoad(cache);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);

  const auto& loaded = *(*result)[0];
  EXPECT_EQ(loaded.key, "chat/claude-3-haiku");
  EXPECT_EQ(loaded.display_name, "claude-haiku-20240307 Display");
  EXPECT_TRUE(loaded.is_suggested_model);
  EXPECT_FALSE(loaded.is_near_model);

  ASSERT_EQ(loaded.supported_capabilities.size(), 2u);
  EXPECT_EQ(loaded.supported_capabilities[0],
            mojom::ConversationCapability::CHAT);
  EXPECT_EQ(loaded.supported_capabilities[1],
            mojom::ConversationCapability::FILES);

  ASSERT_TRUE(loaded.options && loaded.options->is_leo_model_options());
  const auto& leo = loaded.options->get_leo_model_options();
  EXPECT_EQ(leo->name, "claude-haiku-20240307");
  EXPECT_EQ(leo->display_maker, "Anthropic");
  EXPECT_EQ(leo->description, "Fast and capable");
  EXPECT_EQ(leo->access, mojom::ModelAccess::PREMIUM);
  EXPECT_EQ(leo->max_associated_content_length, 90000u);
  EXPECT_EQ(leo->long_conversation_warning_character_limit, 160000u);
}

TEST_F(RemoteModelsDiskCacheTest, SaveOverwritesPreviousCache) {
  auto cache = MakeCache();

  std::vector<mojom::ModelPtr> first;
  first.push_back(MakeTestModel("old-key", "old-name"));
  SaveAndWait(cache, std::move(first));

  std::vector<mojom::ModelPtr> second;
  second.push_back(MakeTestModel("new-key-1", "new-name-1"));
  second.push_back(MakeTestModel("new-key-2", "new-name-2"));
  SaveAndWait(cache, std::move(second));

  auto result = RunLoad(cache);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 2u);
  EXPECT_EQ((*result)[0]->key, "new-key-1");
  EXPECT_EQ((*result)[1]->key, "new-key-2");
}

TEST_F(RemoteModelsDiskCacheTest, AllCapabilitiesRoundTrip) {
  auto cache = MakeCache();

  auto model = MakeTestModel("summary-model", "summary-model-v1");
  model->supported_capabilities = {mojom::ConversationCapability::CHAT,
                                   mojom::ConversationCapability::CONTENT_AGENT,
                                   mojom::ConversationCapability::DEEP_RESEARCH,
                                   mojom::ConversationCapability::FILES,
                                   mojom::ConversationCapability::SUMMARY};

  std::vector<mojom::ModelPtr> save;
  save.push_back(std::move(model));
  SaveAndWait(cache, std::move(save));

  auto result = RunLoad(cache);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);
  EXPECT_EQ((*result)[0]->supported_capabilities.size(), 5u);
}

TEST_F(RemoteModelsDiskCacheTest, AllAccessLevelsRoundTrip) {
  auto cache = MakeCache();

  std::vector<mojom::ModelPtr> save;
  auto basic = MakeTestModel("basic-key", "basic-name");
  basic->options->get_leo_model_options()->access = mojom::ModelAccess::BASIC;
  save.push_back(std::move(basic));

  auto premium = MakeTestModel("premium-key", "premium-name");
  premium->options->get_leo_model_options()->access =
      mojom::ModelAccess::PREMIUM;
  save.push_back(std::move(premium));

  auto both = MakeTestModel("both-key", "both-name");
  both->options->get_leo_model_options()->access =
      mojom::ModelAccess::BASIC_AND_PREMIUM;
  save.push_back(std::move(both));

  SaveAndWait(cache, std::move(save));

  auto result = RunLoad(cache);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 3u);
  EXPECT_EQ((*result)[0]->options->get_leo_model_options()->access,
            mojom::ModelAccess::BASIC);
  EXPECT_EQ((*result)[1]->options->get_leo_model_options()->access,
            mojom::ModelAccess::PREMIUM);
  EXPECT_EQ((*result)[2]->options->get_leo_model_options()->access,
            mojom::ModelAccess::BASIC_AND_PREMIUM);
}

}  // namespace ai_chat
