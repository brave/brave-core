// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"

#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

class FiltersProviderManagerTestObserver
    : public brave_shields::AdBlockFiltersProvider::Observer {
 public:
  FiltersProviderManagerTestObserver() = default;

  // AdBlockFiltersProvider::Observer
  void OnChanged(bool is_default_engine) override { changed_count += 1; }

  int changed_count = 0;
};

// OnChanged notifies observers when all providers are initialized.
TEST(AdBlockFiltersProviderManagerTest, OnChangedNotifiesWhenReady) {
  FiltersProviderManagerTestObserver test_observer;
  brave_shields::AdBlockFiltersProviderManager m;
  m.AddObserver(&test_observer);

  brave_shields::TestFiltersProvider provider1("", true, 0);
  EXPECT_EQ(test_observer.changed_count, 0);
  provider1.RegisterAsSourceProvider(&m);
  EXPECT_EQ(test_observer.changed_count, 1);

  brave_shields::TestFiltersProvider provider2("", true, 0);
  provider2.RegisterAsSourceProvider(&m);
  EXPECT_EQ(test_observer.changed_count, 2);
}

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
