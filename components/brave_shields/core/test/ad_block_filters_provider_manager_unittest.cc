// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"

class FiltersProviderManagerTestObserver
    : public brave_shields::AdBlockFiltersProvider::Observer {
 public:
  FiltersProviderManagerTestObserver() = default;

  // AdBlockFiltersProvider::Observer
  void OnChanged(bool is_default_engine) override { changed_count += 1; }

  int changed_count = 0;
};

TEST(AdBlockFiltersProviderManagerTest, MaybeNotifyObserverNotifiesWhenReady) {
  brave_shields::AdBlockFiltersProviderManager m;

  brave_shields::TestFiltersProvider provider1("rules_a", true, 0);
  provider1.RegisterAsSourceProvider(&m);

  FiltersProviderManagerTestObserver observer;
  m.MaybeNotifyObserver(observer, true);
  EXPECT_EQ(observer.changed_count, 1);

  EXPECT_EQ(m.GetProviders(true).size(), 1u);
}

TEST(AdBlockFiltersProviderManagerTest,
     ForceNotifyObserverDoesNotNotifyWhenNoProviders) {
  brave_shields::AdBlockFiltersProviderManager m;

  FiltersProviderManagerTestObserver observer;
  m.ForceNotifyObserver(observer, true);

  // Should not be notified when there are no providers.
  EXPECT_EQ(observer.changed_count, 0);
}

TEST(AdBlockFiltersProviderManagerTest, ForceNotifyObserverRespectsEngineType) {
  brave_shields::AdBlockFiltersProviderManager m;

  brave_shields::TestFiltersProvider default_provider("default_rules", true, 0);
  default_provider.RegisterAsSourceProvider(&m);

  brave_shields::TestFiltersProvider additional_provider("additional_rules",
                                                         false, 0);
  additional_provider.RegisterAsSourceProvider(&m);

  FiltersProviderManagerTestObserver default_observer;
  m.ForceNotifyObserver(default_observer, true);
  EXPECT_EQ(default_observer.changed_count, 1);

  FiltersProviderManagerTestObserver additional_observer;
  m.ForceNotifyObserver(additional_observer, false);
  EXPECT_EQ(additional_observer.changed_count, 1);
}

TEST(AdBlockFiltersProviderManagerTest,
     OnChangedWaitsForAllProvidersInitialized) {
  brave_shields::AdBlockFiltersProviderManager m;

  FiltersProviderManagerTestObserver observer;
  m.AddObserver(&observer);

  // Register provider1 — it initializes, OnChanged fires, observer notified.
  brave_shields::TestFiltersProvider provider1("rules_a", true, 0);
  provider1.RegisterAsSourceProvider(&m);
  EXPECT_EQ(observer.changed_count, 1);

  // Add provider2 without initializing it.
  brave_shields::TestFiltersProvider provider2("rules_b", true, 0);
  m.AddProvider(&provider2, true);
  EXPECT_EQ(observer.changed_count, 1);

  // Force a notification attempt. provider2 is not initialized,
  // so the manager should NOT propagate to the observer.
  m.ForceNotifyObserver(observer, true);
  EXPECT_EQ(observer.changed_count, 1);

  // Initialize provider2 — all providers initialized, observer notified.
  provider2.Initialize();
  EXPECT_EQ(observer.changed_count, 2);
}

class AdBlockFiltersProviderManagerDATCacheTest
    : public testing::TestWithParam<bool> {
 protected:
  void SetUp() override {
    if (GetParam()) {
      feature_list_.InitAndEnableFeature(
          brave_shields::features::kAdblockDATCache);
    } else {
      feature_list_.InitAndDisableFeature(
          brave_shields::features::kAdblockDATCache);
    }
  }

  base::test::ScopedFeatureList feature_list_;
};

// Disabled: startup OnChanged notifies observer for each provider init.
// Enabled: first provider's OnChanged is suppressed; subsequent providers
// notify normally.
TEST_P(AdBlockFiltersProviderManagerDATCacheTest, OnChangedNotifiesWhenReady) {
  const bool dat_cache_enabled = GetParam();
  brave_shields::AdBlockFiltersProviderManager m;
  FiltersProviderManagerTestObserver observer;
  m.AddObserver(&observer);

  brave_shields::TestFiltersProvider provider1("", true, 0);
  EXPECT_EQ(observer.changed_count, 0);
  provider1.RegisterAsSourceProvider(&m);
  EXPECT_EQ(observer.changed_count, dat_cache_enabled ? 0 : 1);

  brave_shields::TestFiltersProvider provider2("", true, 0);
  provider2.RegisterAsSourceProvider(&m);
  EXPECT_EQ(observer.changed_count, dat_cache_enabled ? 1 : 2);
}

// Disabled: init OnChanged fires (count=1), ForceNotify fires again (count=2).
// Enabled: init OnChanged suppressed (count=0), ForceNotify fires (count=1).
TEST_P(AdBlockFiltersProviderManagerDATCacheTest,
       NotifiesAfterRegisterAndForceNotify) {
  const bool dat_cache_enabled = GetParam();
  brave_shields::AdBlockFiltersProviderManager m;

  FiltersProviderManagerTestObserver observer;
  m.AddObserver(&observer);

  brave_shields::TestFiltersProvider provider("rules", true, 0);
  provider.RegisterAsSourceProvider(&m);
  EXPECT_EQ(observer.changed_count, dat_cache_enabled ? 0 : 1);

  m.ForceNotifyObserver(observer, true);
  EXPECT_EQ(observer.changed_count, dat_cache_enabled ? 1 : 2);
}

// Provider added but not initialized. ForceNotify defers; observer fires when
// provider is later initialized. Same result regardless of flag.
TEST_P(AdBlockFiltersProviderManagerDATCacheTest,
       FiresWhenProviderLaterInitialized) {
  brave_shields::AdBlockFiltersProviderManager m;

  FiltersProviderManagerTestObserver observer;
  m.AddObserver(&observer);

  brave_shields::TestFiltersProvider provider("rules", true, 0);
  m.AddProvider(&provider, true);

  m.ForceNotifyObserver(observer, true);
  EXPECT_EQ(observer.changed_count, 0);

  provider.Initialize();
  EXPECT_EQ(observer.changed_count, 1);
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdBlockFiltersProviderManagerDATCacheTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "DATCacheEnabled"
                                             : "DATCacheDisabled";
                         });
