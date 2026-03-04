/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/time_period_storage/pref_time_period_store.h"

#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kListPrefName[] = "testing.time_period_storage.list";
constexpr char kDictPrefName[] = "testing.time_period_storage.dict";
constexpr char kDictKey[] = "testing_key";

}  // namespace

class PrefTimePeriodStoreTest : public ::testing::Test {
 public:
  PrefTimePeriodStoreTest() {
    pref_service_.registry()->RegisterListPref(kListPrefName);
    pref_service_.registry()->RegisterDictionaryPref(kDictPrefName);
  }
  ~PrefTimePeriodStoreTest() override = default;

  PrefService* pref_service() { return &pref_service_; }

 private:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(PrefTimePeriodStoreTest, SetListPrefStore) {
  PrefTimePeriodStore store(pref_service(), kListPrefName);
  store.Set(base::ListValue().Append(1));

  ASSERT_TRUE(store.Get());
  EXPECT_THAT(*store.Get(), ::testing::ElementsAre(1));
}

TEST_F(PrefTimePeriodStoreTest, SetDictPrefStore) {
  PrefTimePeriodStore store(pref_service(), kDictPrefName, kDictKey);
  store.Set(base::ListValue().Append(1));

  ASSERT_TRUE(store.Get());
  EXPECT_THAT(*store.Get(), ::testing::ElementsAre(1));
}

TEST_F(PrefTimePeriodStoreTest, UpdateListPrefStore) {
  PrefTimePeriodStore store(pref_service(), kListPrefName);
  store.Set(base::ListValue().Append(1));

  // Update the store with new list.
  store.Set(base::ListValue().Append(2).Append(3));

  ASSERT_TRUE(store.Get());
  EXPECT_THAT(*store.Get(), ::testing::ElementsAre(2, 3));
}

TEST_F(PrefTimePeriodStoreTest, UpdateDictPrefStore) {
  PrefTimePeriodStore store(pref_service(), kDictPrefName, kDictKey);
  store.Set(base::ListValue().Append(1));

  // Update the store with new list.
  store.Set(base::ListValue().Append(2).Append(3));

  ASSERT_TRUE(store.Get());
  EXPECT_THAT(*store.Get(), ::testing::ElementsAre(2, 3));
}

TEST_F(PrefTimePeriodStoreTest, ClearListPrefStore) {
  PrefTimePeriodStore store(pref_service(), kListPrefName);
  store.Set(base::ListValue().Append(1));

  store.Clear();

  ASSERT_TRUE(store.Get());
  EXPECT_THAT(*store.Get(), ::testing::IsEmpty());
}

TEST_F(PrefTimePeriodStoreTest, ClearDictPrefStore) {
  PrefTimePeriodStore store(pref_service(), kDictPrefName, kDictKey);
  store.Set(base::ListValue().Append(1));

  store.Clear();

  EXPECT_FALSE(store.Get());
}

TEST_F(PrefTimePeriodStoreTest, GetUninitializedListPrefStore) {
  PrefTimePeriodStore store(pref_service(), kListPrefName);
  ASSERT_TRUE(store.Get());
  EXPECT_THAT(*store.Get(), ::testing::IsEmpty());
}

TEST_F(PrefTimePeriodStoreTest, GetUninitializedDictPrefStore) {
  PrefTimePeriodStore store(pref_service(), kDictPrefName, kDictKey);
  EXPECT_FALSE(store.Get());
}

TEST_F(PrefTimePeriodStoreTest, SetDictPrefStoresWithDifferentKeys) {
  PrefTimePeriodStore store1(pref_service(), kDictPrefName, kDictKey);
  PrefTimePeriodStore store2(pref_service(), kDictPrefName,
                             "other_testing_key");

  store1.Set(base::ListValue().Append(1));
  store2.Set(base::ListValue().Append(2).Append(3));

  ASSERT_TRUE(store1.Get());
  EXPECT_THAT(*store1.Get(), ::testing::ElementsAre(1));
  ASSERT_TRUE(store2.Get());
  EXPECT_THAT(*store2.Get(), ::testing::ElementsAre(2, 3));
}
