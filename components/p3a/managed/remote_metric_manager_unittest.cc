/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/remote_metric_manager.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr char kTestPrefName[] = "test_pref";
constexpr char kTestMetricName[] = "test_metric";

constexpr char kSimplePrefMetricJson[] = R"({
  "type": "pref",
  "pref_name": "test_pref",
  "use_profile_prefs": false
})";

constexpr char kSimpleProfilePrefMetricJson[] = R"({
  "type": "pref",
  "pref_name": "test_pref",
  "use_profile_prefs": true
})";

}  // namespace

class P3ARemoteMetricManagerUnitTest : public testing::Test,
                                       public RemoteMetricManager::Delegate {
 public:
  P3ARemoteMetricManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    local_state_.registry()->RegisterDictionaryPref(
        kRemoteMetricStorageDictPref);
    local_state_.registry()->RegisterIntegerPref(kTestPrefName, 0);

    primary_profile_path_ = base::FilePath(FILE_PATH_LITERAL("profile1"));
    secondary_profile_path_ = base::FilePath(FILE_PATH_LITERAL("profile2"));

    primary_profile_prefs_.registry()->RegisterIntegerPref(kTestPrefName, 0);
    secondary_profile_prefs_.registry()->RegisterIntegerPref(kTestPrefName, 0);

    manager_ = std::make_unique<RemoteMetricManager>(&local_state_, this);
  }

  // RemoteMetricManager::Delegate implementation
  void UpdateMetricValue(std::string_view histogram_name,
                         size_t bucket) override {
    last_updated_metric_ = std::string(histogram_name);
    last_updated_bucket_ = bucket;
    update_count_++;
  }

 protected:
  void AddDefinition(RemoteMetricManager::UnparsedDefinitionsMap& definitions,
                     const std::string& metric_name,
                     std::string_view json) {
    auto definition_value = base::JSONReader::Read(json);
    ASSERT_TRUE(definition_value.has_value());
    definitions[metric_name] =
        std::make_unique<base::Value>(std::move(*definition_value));
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple primary_profile_prefs_;
  TestingPrefServiceSimple secondary_profile_prefs_;
  std::unique_ptr<RemoteMetricManager> manager_;

  base::FilePath primary_profile_path_;
  base::FilePath secondary_profile_path_;

  std::string last_updated_metric_;
  size_t last_updated_bucket_ = 0;
  size_t update_count_ = 0;
};

TEST_F(P3ARemoteMetricManagerUnitTest, ProcessMetricDefinitions) {
  // Set up profile prefs first since they're required
  manager_->HandleProfileLoad(&primary_profile_prefs_, primary_profile_path_,
                              true);

  RemoteMetricManager::UnparsedDefinitionsMap definitions1;

  AddDefinition(definitions1, kTestMetricName, kSimplePrefMetricJson);

  manager_->ProcessMetricDefinitions(std::move(definitions1));

  EXPECT_EQ(manager_->metrics_.size(), 1u);

  RemoteMetricManager::UnparsedDefinitionsMap definitions2;
  AddDefinition(definitions2, "metric1", kSimplePrefMetricJson);
  AddDefinition(definitions2, "metric2", kSimpleProfilePrefMetricJson);

  manager_->ProcessMetricDefinitions(std::move(definitions2));

  EXPECT_EQ(manager_->metrics_.size(), 2u);
}

TEST_F(P3ARemoteMetricManagerUnitTest,
       ProcessMetricDefinitionsBeforeProfileLoad) {
  // Process metric definitions before profile is loaded
  RemoteMetricManager::UnparsedDefinitionsMap definitions;
  AddDefinition(definitions, "metric1", kSimplePrefMetricJson);
  AddDefinition(definitions, "metric2", kSimpleProfilePrefMetricJson);

  manager_->ProcessMetricDefinitions(std::move(definitions));

  // No metrics should be instantiated yet since profile prefs aren't available
  EXPECT_EQ(manager_->metrics_.size(), 0u);

  // Now load the profile - this should trigger processing of the stored
  // definitions
  manager_->HandleProfileLoad(&primary_profile_prefs_, primary_profile_path_,
                              true);

  // Metrics should now be instantiated
  EXPECT_EQ(manager_->metrics_.size(), 2u);
}

TEST_F(P3ARemoteMetricManagerUnitTest, MetricReported) {
  // Set up profile prefs first since they're required
  manager_->HandleProfileLoad(&primary_profile_prefs_, primary_profile_path_,
                              true);

  EXPECT_EQ(update_count_, 0u);
  EXPECT_EQ(last_updated_bucket_, 0u);

  // Create a simple pref metric
  RemoteMetricManager::UnparsedDefinitionsMap definitions;
  AddDefinition(definitions, kTestMetricName, kSimpleProfilePrefMetricJson);
  manager_->ProcessMetricDefinitions(std::move(definitions));

  EXPECT_EQ(update_count_, 1u);
  EXPECT_EQ(last_updated_metric_, kTestMetricName);
  EXPECT_EQ(last_updated_bucket_, 0u);

  primary_profile_prefs_.SetInteger(kTestPrefName, 1);

  EXPECT_EQ(update_count_, 2u);
  EXPECT_EQ(last_updated_metric_, kTestMetricName);
  EXPECT_EQ(last_updated_bucket_, 1u);

  manager_->HandleProfileLoad(&secondary_profile_prefs_,
                              secondary_profile_path_, true);

  EXPECT_EQ(update_count_, 3u);
  EXPECT_EQ(last_updated_metric_, kTestMetricName);
  EXPECT_EQ(last_updated_bucket_, 0u);

  secondary_profile_prefs_.SetInteger(kTestPrefName, 2);

  EXPECT_EQ(update_count_, 4u);
  EXPECT_EQ(last_updated_metric_, kTestMetricName);
  EXPECT_EQ(last_updated_bucket_, 2u);
}

TEST_F(P3ARemoteMetricManagerUnitTest, InvalidMetricDefinitionsAreSkipped) {
  // Set up profile prefs first since they're required
  manager_->HandleProfileLoad(&primary_profile_prefs_, primary_profile_path_,
                              true);

  RemoteMetricManager::UnparsedDefinitionsMap definitions;

  // Add a valid metric
  AddDefinition(definitions, "valid_metric", kSimplePrefMetricJson);

  // Add an invalid metric missing the type
  AddDefinition(definitions, "invalid_type", "{}");

  // Add an invalid metric with unknown type
  AddDefinition(definitions, "invalid_unknown_type",
                R"({"type": "unknown_type"})");

  // Add an invalid metric with missing pref name
  AddDefinition(definitions, "invalid_pref", R"({"type": "pref"})");

  manager_->ProcessMetricDefinitions(std::move(definitions));

  // Only the valid metric should be processed
  EXPECT_EQ(manager_->metrics_.size(), 1u);
}

TEST_F(P3ARemoteMetricManagerUnitTest, ProfilePrefsHandling) {
  EXPECT_EQ(manager_->last_used_profile_prefs_, nullptr);

  // Load primary profile as the last used profile
  manager_->HandleProfileLoad(&primary_profile_prefs_, primary_profile_path_,
                              true);
  EXPECT_EQ(manager_->last_used_profile_prefs_, &primary_profile_prefs_);

  // Load secondary profile (not as last used)
  manager_->HandleProfileLoad(&secondary_profile_prefs_,
                              secondary_profile_path_, false);
  EXPECT_EQ(manager_->last_used_profile_prefs_, &primary_profile_prefs_);

  // Switch to secondary profile as last used
  manager_->HandleLastUsedProfileChanged(secondary_profile_path_);
  EXPECT_EQ(manager_->last_used_profile_prefs_, &secondary_profile_prefs_);

  // Unload secondary profile
  manager_->HandleProfileUnload(secondary_profile_path_);
  EXPECT_EQ(manager_->last_used_profile_prefs_, nullptr);
}

TEST_F(P3ARemoteMetricManagerUnitTest, CleanupStorage) {
  // Set up profile prefs first since they're required
  manager_->HandleProfileLoad(&primary_profile_prefs_, primary_profile_path_,
                              true);

  RemoteMetricManager::UnparsedDefinitionsMap definitions;

  AddDefinition(definitions, kTestMetricName, kSimplePrefMetricJson);

  manager_->ProcessMetricDefinitions(std::move(definitions));

  {
    ScopedDictPrefUpdate update(&local_state_, kRemoteMetricStorageDictPref);
    update->Set("unused_key", 1);
  }

  manager_->ProcessMetricDefinitions({});

  const auto& storage = local_state_.GetDict(kRemoteMetricStorageDictPref);
  EXPECT_FALSE(storage.FindDict({}));
}

}  // namespace p3a
