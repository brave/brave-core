// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"

#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

class TestFiltersProviderManagerObserver
    : public AdBlockFiltersProvider::Observer {
 public:
  void OnChanged(bool is_default_engine) override {
    if (is_default_engine) {
      default_changed_count++;
    } else {
      additional_changed_count++;
    }
  }

  int default_changed_count = 0;
  int additional_changed_count = 0;
};

}  // namespace

TEST(AdBlockComponentServiceManagerTest, EmptyCatalogInitializesGates) {
  TestingPrefServiceSimple prefs;
  RegisterPrefsForAdBlockService(prefs.registry());

  AdBlockFiltersProviderManager filters_provider_manager;
  AdBlockFilterListCatalogProvider catalog_provider(nullptr);
  AdBlockListP3A list_p3a(&prefs);

  AdBlockComponentServiceManager service_manager(
      &prefs, &filters_provider_manager, "en", nullptr, &catalog_provider,
      &list_p3a);

  TestFiltersProviderManagerObserver observer;
  filters_provider_manager.AddObserver(&observer);

  // Simulate the filter list catalog component providing an empty catalog.
  // Without the fix, StartRegionalServices returns early and leaves the gate
  // providers uninitialized, which permanently blocks filter set loading.
  service_manager.SetFilterListCatalog({});

  // Gates are initialized synchronously, which propagates through the
  // AdBlockFiltersProviderManager to the external observer.
  EXPECT_GE(observer.default_changed_count, 1);
  EXPECT_GE(observer.additional_changed_count, 1);

  filters_provider_manager.RemoveObserver(&observer);
}

}  // namespace brave_shields
