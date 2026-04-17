// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"

#include "base/containers/flat_set.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

bool AllProvidersInitialized(
    const base::flat_set<AdBlockFiltersProvider*>& providers) {
  for (auto* provider : providers) {
    if (!provider->IsInitialized()) {
      return false;
    }
  }
  return true;
}

}  // namespace

class AdBlockComponentServiceManagerTest : public testing::TestWithParam<bool> {
 protected:
  bool IsDATCacheEnabled() const { return GetParam(); }
};

TEST_P(AdBlockComponentServiceManagerTest, EmptyCatalogInitializesGates) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatureState(features::kAdblockDATCache,
                                    IsDATCacheEnabled());

  TestingPrefServiceSimple prefs;
  RegisterPrefsForAdBlockService(prefs.registry());

  AdBlockFiltersProviderManager filters_provider_manager;
  AdBlockFilterListCatalogProvider catalog_provider(nullptr);
  AdBlockListP3A list_p3a(&prefs);

  AdBlockComponentServiceManager service_manager(
      &prefs, &filters_provider_manager, "en", nullptr, &catalog_provider,
      &list_p3a);

  const auto& default_providers =
      filters_provider_manager.GetProviders(/*is_for_default_engine=*/true);
  const auto& additional_providers =
      filters_provider_manager.GetProviders(/*is_for_default_engine=*/false);

  if (IsDATCacheEnabled()) {
    // With the feature enabled, component provider gates are registered but
    // report IsInitialized() == false until the catalog arrives.
    ASSERT_FALSE(default_providers.empty());
    ASSERT_FALSE(additional_providers.empty());
    EXPECT_FALSE(AllProvidersInitialized(default_providers));
    EXPECT_FALSE(AllProvidersInitialized(additional_providers));
  } else {
    // With the feature disabled, no gates exist.
    EXPECT_TRUE(default_providers.empty());
    EXPECT_TRUE(additional_providers.empty());
  }

  // Simulate the filter list catalog component providing an empty catalog.
  // Without the fix, StartRegionalServices returns early and leaves the gate
  // providers uninitialized, which permanently blocks filter set loading.
  service_manager.SetFilterListCatalog({});

  if (IsDATCacheEnabled()) {
    EXPECT_TRUE(AllProvidersInitialized(default_providers));
    EXPECT_TRUE(AllProvidersInitialized(additional_providers));
  } else {
    EXPECT_TRUE(default_providers.empty());
    EXPECT_TRUE(additional_providers.empty());
  }
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdBlockComponentServiceManagerTest,
                         testing::Bool(),
                         [](const auto& info) {
                           return info.param ? "DATCacheEnabled"
                                             : "DATCacheDisabled";
                         });

}  // namespace brave_shields
