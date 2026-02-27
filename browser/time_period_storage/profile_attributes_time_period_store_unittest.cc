/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/time_period_storage/profile_attributes_time_period_store.h"

#include "base/files/file_path.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kMetricName[] = "testing_metric";
constexpr char kDictKey[] = "testing_key";
constexpr base::FilePath::CharType kUserDataDir[] =
    FILE_PATH_LITERAL("/testing_user_data_dir");

}  // namespace

class ProfileAttributesTimePeriodStoreTest : public ::testing::Test {
 public:
  ProfileAttributesTimePeriodStoreTest()
      : profile_path_(
            base::FilePath(kUserDataDir).AppendASCII("testing_profile")) {}

  ~ProfileAttributesTimePeriodStoreTest() override = default;

  void SetUp() override {
    ProfileAttributesStorage::RegisterPrefs(local_state_.registry());
    storage_ = std::make_unique<ProfileAttributesStorage>(
        &local_state_, base::FilePath(kUserDataDir));
    ProfileAttributesInitParams profile_init_params;
    profile_init_params.profile_path = profile_path_;
    storage_->AddProfile(std::move(profile_init_params));
  }

  base::FilePath profile_path() { return profile_path_; }

  ProfileAttributesStorage& profile_attributes_storage() { return *storage_; }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  const base::FilePath profile_path_;
  std::unique_ptr<ProfileAttributesStorage> storage_;
};

TEST_F(ProfileAttributesTimePeriodStoreTest, SetStore) {
  ProfileAttributesTimePeriodStore store(
      profile_path(), profile_attributes_storage(), kMetricName, kDictKey);

  base::ListValue data;
  data.Append(1);
  store.Set(std::move(data));

  ASSERT_TRUE(store.Get());
  EXPECT_THAT(*store.Get(), ::testing::ElementsAre(1));
}

TEST_F(ProfileAttributesTimePeriodStoreTest, UpdateStore) {
  ProfileAttributesTimePeriodStore store(
      profile_path(), profile_attributes_storage(), kMetricName, kDictKey);
  store.Set(base::ListValue().Append(1));

  // Update the store with new data.
  store.Set(base::ListValue().Append(2).Append(3));

  ASSERT_TRUE(store.Get());
  EXPECT_THAT(*store.Get(), ::testing::ElementsAre(2, 3));
}

TEST_F(ProfileAttributesTimePeriodStoreTest, ClearStore) {
  ProfileAttributesTimePeriodStore store(
      profile_path(), profile_attributes_storage(), kMetricName, kDictKey);
  store.Set(base::ListValue().Append(1));

  store.Clear();

  EXPECT_FALSE(store.Get());
}

TEST_F(ProfileAttributesTimePeriodStoreTest, GetUninitializedStore) {
  ProfileAttributesTimePeriodStore store(
      profile_path(), profile_attributes_storage(), kMetricName, kDictKey);
  EXPECT_FALSE(store.Get());
}

TEST_F(ProfileAttributesTimePeriodStoreTest, SetStoresWithDifferentKeys) {
  ProfileAttributesTimePeriodStore store1(
      profile_path(), profile_attributes_storage(), kMetricName, kDictKey);
  ProfileAttributesTimePeriodStore store2(profile_path(),
                                          profile_attributes_storage(),
                                          kMetricName, "other_testing_key");

  store1.Set(base::ListValue().Append(1));
  store2.Set(base::ListValue().Append(2).Append(3));

  ASSERT_TRUE(store1.Get());
  EXPECT_THAT(*store1.Get(), ::testing::ElementsAre(1));
  ASSERT_TRUE(store2.Get());
  EXPECT_THAT(*store2.Get(), ::testing::ElementsAre(2, 3));
}
