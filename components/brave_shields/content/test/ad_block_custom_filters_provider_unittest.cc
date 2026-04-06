// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"

#include <string>

#include "base/time/time.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class AdBlockCustomFiltersProviderTest : public testing::Test {
 public:
  void SetUp() override {
    prefs_.registry()->RegisterStringPref(
        brave_shields::prefs::kAdBlockCustomFilters, std::string());
    prefs_.registry()->RegisterTimePref(
        brave_shields::prefs::kAdBlockCustomFiltersLastModified, base::Time());
  }

 protected:
  TestingPrefServiceSimple prefs_;
};

TEST_F(AdBlockCustomFiltersProviderTest, TimestampUpdatedOnFilterChange) {
  brave_shields::AdBlockCustomFiltersProvider provider(&prefs_, nullptr);

  // Initially timestamp should be unset.
  EXPECT_EQ(provider.GetTimestamp(), base::Time());

  provider.UpdateCustomFilters("||example.com^");

  // After update, timestamp should be recent (not zero).
  EXPECT_NE(provider.GetTimestamp(), base::Time());
  EXPECT_LE(provider.GetTimestamp(), base::Time::Now());
}

TEST_F(AdBlockCustomFiltersProviderTest, TimestampAdvancesOnSubsequentUpdates) {
  brave_shields::AdBlockCustomFiltersProvider provider(&prefs_, nullptr);

  provider.UpdateCustomFilters("||first.com^");
  base::Time first_timestamp = provider.GetTimestamp();
  ASSERT_NE(first_timestamp, base::Time());

  provider.UpdateCustomFilters("||second.com^");
  base::Time second_timestamp = provider.GetTimestamp();

  EXPECT_GE(second_timestamp, first_timestamp);
}

TEST_F(AdBlockCustomFiltersProviderTest,
       NotifiesObserversWithTimestampOnUpdate) {
  brave_shields::AdBlockCustomFiltersProvider provider(&prefs_, nullptr);

  class TestObserver : public brave_shields::AdBlockFiltersProvider::Observer {
   public:
    void OnChanged(bool is_default_engine, base::Time timestamp) override {
      notified = true;
      last_timestamp = timestamp;
    }
    bool notified = false;
    base::Time last_timestamp;
  };

  TestObserver observer;
  provider.AddObserver(&observer);

  provider.UpdateCustomFilters("||notify-test.com^");

  EXPECT_TRUE(observer.notified);
  EXPECT_NE(observer.last_timestamp, base::Time());
  EXPECT_EQ(observer.last_timestamp, provider.GetTimestamp());

  provider.RemoveObserver(&observer);
}
