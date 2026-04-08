// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"

#include <string>

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
  }

 protected:
  TestingPrefServiceSimple prefs_;
};

TEST_F(AdBlockCustomFiltersProviderTest, HashUpdatedOnFilterChange) {
  brave_shields::AdBlockCustomFiltersProvider provider(&prefs_, nullptr);

  // Initially hash should be the hash of empty string (not empty itself).
  std::string initial_hash = provider.GetCacheKey().value();
  EXPECT_FALSE(initial_hash.empty());

  provider.UpdateCustomFilters("||example.com^");

  // After update, hash should differ from the initial hash.
  EXPECT_NE(provider.GetCacheKey().value(), initial_hash);
}

TEST_F(AdBlockCustomFiltersProviderTest, HashChangesWithDifferentContent) {
  brave_shields::AdBlockCustomFiltersProvider provider(&prefs_, nullptr);

  provider.UpdateCustomFilters("||first.com^");
  std::string first_hash = provider.GetCacheKey().value();
  ASSERT_FALSE(first_hash.empty());

  provider.UpdateCustomFilters("||second.com^");
  std::string second_hash = provider.GetCacheKey().value();

  EXPECT_NE(second_hash, first_hash);
}

TEST_F(AdBlockCustomFiltersProviderTest, NotifiesObserversWithHashOnUpdate) {
  brave_shields::AdBlockCustomFiltersProvider provider(&prefs_, nullptr);

  class TestObserver : public brave_shields::AdBlockFiltersProvider::Observer {
   public:
    void OnChanged(bool is_default_engine) override { notified = true; }
    bool notified = false;
  };

  TestObserver observer;
  provider.AddObserver(&observer);

  provider.UpdateCustomFilters("||notify-test.com^");

  EXPECT_TRUE(observer.notified);

  provider.RemoveObserver(&observer);
}
