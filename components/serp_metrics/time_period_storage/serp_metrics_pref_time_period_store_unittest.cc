/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store.h"

#include <string_view>

#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

namespace {

constexpr std::string_view kPrefName = "baz";
constexpr std::string_view kFooPrefKey = "foo";
constexpr std::string_view kBarPrefKey = "bar";

}  // namespace

class SerpMetricsPrefTimePeriodStoreTest : public ::testing::Test {
 public:
  TestingPrefServiceSimple& pref_service() { return pref_service_; }

 private:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(SerpMetricsPrefTimePeriodStoreTest, SetStore) {
  // Arrange
  pref_service().registry()->RegisterDictionaryPref(kPrefName);
  SerpMetricsPrefTimePeriodStore store(&pref_service(), kPrefName, kFooPrefKey);

  // Act
  store.Set(base::ListValue().Append(1));
  ASSERT_NE(nullptr, store.Get());

  // Assert
  EXPECT_THAT(*store.Get(), ::testing::ElementsAre(1));
}

TEST_F(SerpMetricsPrefTimePeriodStoreTest, UpdateStore) {
  // Arrange
  pref_service().registry()->RegisterDictionaryPref(kPrefName);
  SerpMetricsPrefTimePeriodStore store(&pref_service(), kPrefName, kFooPrefKey);
  store.Set(base::ListValue().Append(1));

  // Act
  store.Set(base::ListValue().Append(2).Append(3));
  ASSERT_NE(nullptr, store.Get());

  // Assert
  EXPECT_THAT(*store.Get(), ::testing::ElementsAre(2, 3));
}

TEST_F(SerpMetricsPrefTimePeriodStoreTest, ClearStore) {
  // Arrange
  pref_service().registry()->RegisterDictionaryPref(kPrefName);
  SerpMetricsPrefTimePeriodStore store(&pref_service(), kPrefName, kFooPrefKey);
  store.Set(base::ListValue().Append(1));

  // Act
  store.Clear();

  // Assert
  EXPECT_FALSE(store.Get());
}

TEST_F(SerpMetricsPrefTimePeriodStoreTest, GetUninitializedStore) {
  // Arrange
  pref_service().registry()->RegisterDictionaryPref(kPrefName);
  SerpMetricsPrefTimePeriodStore store(&pref_service(), kPrefName, kFooPrefKey);

  // Assert
  EXPECT_FALSE(store.Get());
}

TEST_F(SerpMetricsPrefTimePeriodStoreTest,
       StoresWithDifferentKeysAreIndependent) {
  // Arrange
  pref_service().registry()->RegisterDictionaryPref(kPrefName);
  SerpMetricsPrefTimePeriodStore store(&pref_service(), kPrefName, kFooPrefKey);
  SerpMetricsPrefTimePeriodStore other_store(&pref_service(), kPrefName,
                                             kBarPrefKey);

  // Act
  store.Set(base::ListValue().Append(1));
  other_store.Set(base::ListValue().Append(2).Append(3));

  ASSERT_NE(nullptr, store.Get());
  ASSERT_NE(nullptr, other_store.Get());

  // Assert
  EXPECT_THAT(*store.Get(), ::testing::ElementsAre(1));
  EXPECT_THAT(*other_store.Get(), ::testing::ElementsAre(2, 3));
}

TEST_F(SerpMetricsPrefTimePeriodStoreTest, ClearUninitializedStore) {
  // Arrange
  pref_service().registry()->RegisterDictionaryPref(kPrefName);
  SerpMetricsPrefTimePeriodStore store(&pref_service(), kPrefName, kFooPrefKey);

  // Act
  store.Clear();

  // Assert
  EXPECT_FALSE(store.Get());
}

TEST_F(SerpMetricsPrefTimePeriodStoreTest, ClearStoreDoesNotAffectOtherKeys) {
  // Arrange
  pref_service().registry()->RegisterDictionaryPref(kPrefName);
  SerpMetricsPrefTimePeriodStore store(&pref_service(), kPrefName, kFooPrefKey);
  SerpMetricsPrefTimePeriodStore other_store(&pref_service(), kPrefName,
                                             kBarPrefKey);
  store.Set(base::ListValue().Append(1));
  other_store.Set(base::ListValue().Append(2).Append(3));

  // Act
  store.Clear();

  EXPECT_FALSE(store.Get());
  ASSERT_NE(nullptr, other_store.Get());

  // Assert
  EXPECT_THAT(*other_store.Get(), ::testing::ElementsAre(2, 3));
}

}  // namespace serp_metrics
