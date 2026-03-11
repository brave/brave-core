// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/prefs.h"

#include <utility>

#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/browser/prefs_registration.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom-data-view.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class ContainersPrefsTest : public testing::Test {
 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kContainers);
    RegisterProfilePrefs(prefs_.registry());
  }

  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(ContainersPrefsTest, GetEmptyContainerList) {
  auto containers = GetContainersFromPrefs(prefs_);
  EXPECT_TRUE(containers.empty());
}

TEST_F(ContainersPrefsTest, SetAndGetContainerList) {
  std::vector<mojom::ContainerPtr> test_containers;
  test_containers.push_back(mojom::Container::New(
      "test-id-1", "Test Container 1", mojom::Icon::kPersonal, SK_ColorWHITE));
  test_containers.push_back(mojom::Container::New(
      "test-id-2", "Test Container 2", mojom::Icon::kWork, SK_ColorBLACK));

  SetContainersToPrefs(test_containers, prefs_);

  auto retrieved_containers = GetContainersFromPrefs(prefs_);
  ASSERT_EQ(retrieved_containers.size(), 2u);

  EXPECT_EQ(retrieved_containers[0]->id, "test-id-1");
  EXPECT_EQ(retrieved_containers[0]->name, "Test Container 1");
  EXPECT_EQ(retrieved_containers[0]->icon, mojom::Icon::kPersonal);
  EXPECT_EQ(retrieved_containers[0]->background_color, SK_ColorWHITE);

  EXPECT_EQ(retrieved_containers[1]->id, "test-id-2");
  EXPECT_EQ(retrieved_containers[1]->name, "Test Container 2");
  EXPECT_EQ(retrieved_containers[1]->icon, mojom::Icon::kWork);
  EXPECT_EQ(retrieved_containers[1]->background_color, SK_ColorBLACK);
}

TEST_F(ContainersPrefsTest, GetContainerListInvalidData) {
  // Test with invalid list items
  base::ListValue invalid_list;
  invalid_list.Append(base::Value(42));  // Not a dictionary
  invalid_list.Append(
      base::DictValue()
          // Missing name field
          .Set("id", "test-id")
          .Set("icon", std::to_underlying(mojom::Icon::kPersonal))
          .Set("background_color", static_cast<int>(SK_ColorWHITE)));

  invalid_list.Append(
      base::DictValue()
          // Missing id field
          .Set("name", "Test Container")
          .Set("icon", std::to_underlying(mojom::Icon::kPersonal))
          .Set("background_color", static_cast<int>(SK_ColorWHITE)));

  invalid_list.Append(
      base::DictValue()
          // Missing icon field
          .Set("id", "test-id")
          .Set("name", "Test Container")
          .Set("background_color", static_cast<int>(SK_ColorWHITE)));

  invalid_list.Append(
      base::DictValue()
          // Missing background_color field
          .Set("id", "test-id")
          .Set("name", "Test Container")
          .Set("icon", std::to_underlying(mojom::Icon::kPersonal)));

  prefs_.SetList(prefs::kContainersList, std::move(invalid_list));

  auto containers = GetContainersFromPrefs(prefs_);
  EXPECT_TRUE(containers.empty());
}

TEST_F(ContainersPrefsTest, SetContainerListEmpty) {
  std::vector<mojom::ContainerPtr> empty_containers;
  SetContainersToPrefs(empty_containers, prefs_);

  const base::ListValue& list = prefs_.GetList(prefs::kContainersList);
  EXPECT_TRUE(list.empty());
}

TEST_F(ContainersPrefsTest, GetContainerById) {
  std::vector<mojom::ContainerPtr> test_containers;
  test_containers.push_back(mojom::Container::New(
      "test-id-1", "Test Container 1", mojom::Icon::kPersonal, SK_ColorWHITE));
  test_containers.push_back(mojom::Container::New(
      "test-id-2", "Test Container 2", mojom::Icon::kWork, SK_ColorBLACK));
  SetContainersToPrefs(test_containers, prefs_);

  auto container = GetContainerFromPrefs(prefs_, "test-id-2");
  ASSERT_TRUE(container);
  EXPECT_EQ(container->name, "Test Container 2");
  EXPECT_EQ(container->icon, mojom::Icon::kWork);

  EXPECT_FALSE(GetContainerFromPrefs(prefs_, "missing-id"));
}

TEST_F(ContainersPrefsTest, SetAndGetUsedContainer) {
  auto container = mojom::Container::New("used-id", "Used Container",
                                         mojom::Icon::kShopping, SK_ColorBLUE);
  EXPECT_FALSE(HasUsedContainerInPrefs(prefs_, "used-id"));
  SetUsedContainerToPrefs(container, prefs_);
  EXPECT_TRUE(HasUsedContainerInPrefs(prefs_, "used-id"));
  EXPECT_FALSE(HasUsedContainerInPrefs(prefs_, "other-id"));

  auto retrieved = GetUsedContainerFromPrefs(prefs_, "used-id");
  ASSERT_TRUE(retrieved);
  EXPECT_EQ(retrieved->name, "Used Container");
  EXPECT_EQ(retrieved->icon, mojom::Icon::kShopping);
  EXPECT_EQ(retrieved->background_color, SK_ColorBLUE);

  auto all_used = GetUsedContainersFromPrefs(prefs_);
  ASSERT_EQ(all_used.size(), 1u);
  EXPECT_EQ(all_used[0]->id, "used-id");
}

TEST_F(ContainersPrefsTest, UpdateAndRemoveUsedContainer) {
  SetUsedContainerToPrefs(
      mojom::Container::New("used-id", "Used Container", mojom::Icon::kShopping,
                            SK_ColorBLUE),
      prefs_);
  SetUsedContainerToPrefs(
      mojom::Container::New("used-id", "Updated Container", mojom::Icon::kWork,
                            SK_ColorRED),
      prefs_);

  auto retrieved = GetUsedContainerFromPrefs(prefs_, "used-id");
  ASSERT_TRUE(retrieved);
  EXPECT_EQ(retrieved->name, "Updated Container");
  EXPECT_EQ(retrieved->icon, mojom::Icon::kWork);
  EXPECT_EQ(retrieved->background_color, SK_ColorRED);

  RemoveUsedContainerFromPrefs("used-id", prefs_);
  EXPECT_FALSE(GetUsedContainerFromPrefs(prefs_, "used-id"));
  EXPECT_TRUE(GetUsedContainersFromPrefs(prefs_).empty());
}

}  // namespace containers
