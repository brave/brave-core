// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/prefs.h"

#include <utility>

#include "base/test/scoped_feature_list.h"
#include "base/types/cxx23_to_underlying.h"
#include "base/values.h"
#include "brave/components/containers/core/browser/pref_names.h"
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

TEST_F(ContainersPrefsTest, GetEmptyContainersDict) {
  auto containers = GetContainersFromPrefs(prefs_);
  EXPECT_TRUE(containers.empty());
}

TEST_F(ContainersPrefsTest, SetAndGetContainersDict) {
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

TEST_F(ContainersPrefsTest, GetContainersDictInvalidData) {
  // Test with invalid dict items
  base::Value::Dict invalid_dict;
  invalid_dict.Set("test-id-1", base::Value(42));  // Not a dictionary
  invalid_dict.Set(
      "test-id-2",
      base::Value::Dict()
          // Missing name field
          .Set("icon", base::to_underlying(mojom::Icon::kPersonal))
          .Set("background_color", static_cast<int>(SK_ColorWHITE)));

  invalid_dict.Set("test-id-3", base::Value::Dict()
                                    // Missing icon field
                                    .Set("name", "Test Container")
                                    .Set("background_color",
                                         static_cast<int>(SK_ColorWHITE)));

  invalid_dict.Set(
      "test-id-4",
      base::Value::Dict()
          // Missing background_color field
          .Set("name", "Test Container")
          .Set("icon", base::to_underlying(mojom::Icon::kPersonal)));

  prefs_.SetDict(prefs::kContainersDict, std::move(invalid_dict));

  auto containers = GetContainersFromPrefs(prefs_);
  EXPECT_TRUE(containers.empty());
}

TEST_F(ContainersPrefsTest, SetContainersDictEmpty) {
  std::vector<mojom::ContainerPtr> empty_containers;
  SetContainersToPrefs(empty_containers, prefs_);

  const base::Value::Dict& dict = prefs_.GetDict(prefs::kContainersDict);
  EXPECT_TRUE(dict.empty());
}

}  // namespace containers
