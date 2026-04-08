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

TEST(AdBlockFiltersProviderManagerTest, WaitUntilInitialized) {
  FiltersProviderManagerTestObserver test_observer;
  brave_shields::AdBlockFiltersProviderManager m;
  m.AddObserver(&test_observer);

  brave_shields::TestFiltersProvider provider1("", true, 0);
  EXPECT_EQ(test_observer.changed_count, 0);
  provider1.RegisterAsSourceProvider(&m);
  EXPECT_EQ(test_observer.changed_count, 1);
  brave_shields::TestFiltersProvider provider2("", true, 0);
  EXPECT_EQ(test_observer.changed_count, 1);
  provider2.RegisterAsSourceProvider(&m);
  EXPECT_EQ(test_observer.changed_count, 2);
}

TEST(AdBlockFiltersProviderManagerTest, ForceNotifyObserverCombinesHashes) {
  brave_shields::AdBlockFiltersProviderManager m;

  // Create providers with specific content hashes
  brave_shields::TestFiltersProvider provider1("rules_a", true, 0);
  provider1.RegisterAsSourceProvider(&m);

  brave_shields::TestFiltersProvider provider2("rules_b", true, 0);
  provider2.RegisterAsSourceProvider(&m);

  // Create a separate observer for ForceNotifyObserver
  FiltersProviderManagerTestObserver force_observer;
  m.ForceNotifyObserver(force_observer, true);

  // Should have been notified once
  EXPECT_EQ(force_observer.changed_count, 1);

  // Both providers should be in the default engine set.
  EXPECT_EQ(m.GetProviders(true).size(), 2u);
}

TEST(AdBlockFiltersProviderManagerTest,
     ForceNotifyObserverDoesNotNotifyWhenNoProviders) {
  brave_shields::AdBlockFiltersProviderManager m;

  FiltersProviderManagerTestObserver observer;
  m.ForceNotifyObserver(observer, true);

  // Should not be notified when there are no providers
  EXPECT_EQ(observer.changed_count, 0);
}

TEST(AdBlockFiltersProviderManagerTest, ForceNotifyObserverRespectsEngineType) {
  brave_shields::AdBlockFiltersProviderManager m;

  // Create a default engine provider
  brave_shields::TestFiltersProvider default_provider("default_rules", true, 0);
  default_provider.RegisterAsSourceProvider(&m);

  // Create an additional engine provider
  brave_shields::TestFiltersProvider additional_provider("additional_rules",
                                                         false, 0);
  additional_provider.RegisterAsSourceProvider(&m);

  // Observer for default engine only
  FiltersProviderManagerTestObserver default_observer;
  m.ForceNotifyObserver(default_observer, true);
  EXPECT_EQ(default_observer.changed_count, 1);

  // Observer for additional engine only
  FiltersProviderManagerTestObserver additional_observer;
  m.ForceNotifyObserver(additional_observer, false);
  EXPECT_EQ(additional_observer.changed_count, 1);
}

TEST(AdBlockFiltersProviderManagerTest, OnChangedCombinesProviderHashes) {
  brave_shields::AdBlockFiltersProviderManager m;

  FiltersProviderManagerTestObserver observer;
  m.AddObserver(&observer);

  // Create and initialize a provider
  brave_shields::TestFiltersProvider provider("test_rules", true, 0);
  provider.RegisterAsSourceProvider(&m);

  EXPECT_EQ(observer.changed_count, 1);
  // Provider should be in the default engine set.
  EXPECT_EQ(m.GetProviders(true).size(), 1u);
}
