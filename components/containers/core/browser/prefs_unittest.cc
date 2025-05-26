// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/prefs.h"

#include <utility>

#include "base/values.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class ContainersPrefsTest : public testing::Test {
 protected:
  void SetUp() override { RegisterProfilePrefs(prefs_.registry()); }

  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(ContainersPrefsTest, GetEmptyContainerList) {
  auto containers = GetContainerList(prefs_);
  EXPECT_TRUE(containers.empty());
}

TEST_F(ContainersPrefsTest, SetAndGetContainerList) {
  std::vector<mojom::ContainerPtr> test_containers;
  test_containers.push_back(
      mojom::Container::New("test-id-1", "Test Container 1"));
  test_containers.push_back(
      mojom::Container::New("test-id-2", "Test Container 2"));

  SetContainerList(test_containers, prefs_);

  auto retrieved_containers = GetContainerList(prefs_);
  ASSERT_EQ(retrieved_containers.size(), 2u);

  EXPECT_EQ(retrieved_containers[0]->id, "test-id-1");
  EXPECT_EQ(retrieved_containers[0]->name, "Test Container 1");
  EXPECT_EQ(retrieved_containers[1]->id, "test-id-2");
  EXPECT_EQ(retrieved_containers[1]->name, "Test Container 2");
}

TEST_F(ContainersPrefsTest, GetContainerListInvalidData) {
  // Test with invalid list items
  base::Value::List invalid_list;
  invalid_list.Append(base::Value(42));  // Not a dictionary
  invalid_list.Append(base::Value::Dict().Set("id", "test-id")
                      // Missing name field
  );
  invalid_list.Append(base::Value::Dict()
                          // Missing id field
                          .Set("name", "Test Container"));

  prefs_.SetList(prefs::kContainersList, std::move(invalid_list));

  auto containers = GetContainerList(prefs_);
  EXPECT_TRUE(containers.empty());
}

TEST_F(ContainersPrefsTest, SetContainerListEmpty) {
  std::vector<mojom::ContainerPtr> empty_containers;
  SetContainerList(empty_containers, prefs_);

  const base::Value::List& list = prefs_.GetList(prefs::kContainersList);
  EXPECT_TRUE(list.empty());
}

}  // namespace containers
