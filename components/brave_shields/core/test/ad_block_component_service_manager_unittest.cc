// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

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

class CountingObserver : public AdBlockFiltersProvider::Observer {
 public:
  void OnChanged(bool is_for_default_engine) override {
    is_for_default_engine ? ++default_count : ++additional_count;
  }
  int default_count = 0;
  int additional_count = 0;
};

FilterListCatalogEntry MakeCatalogEntry(const std::string& uuid) {
  return FilterListCatalogEntry(uuid, "https://example.com/" + uuid,
                                "Title " + uuid, /*langs=*/{},
                                /*support_url=*/"", /*desc=*/"",
                                /*hidden=*/false, /*default_enabled=*/true,
                                /*first_party_protections=*/true,
                                /*permission_mask=*/0, /*platforms=*/{},
                                /*component_id=*/"cid-" + uuid,
                                /*base64_public_key=*/"");
}

std::vector<FilterListCatalogEntry> MakeCatalog(size_t size) {
  std::vector<FilterListCatalogEntry> entries;
  for (size_t i = 0; i < size; ++i) {
    entries.push_back(MakeCatalogEntry(absl::StrFormat("uuid-%zu", i)));
  }
  return entries;
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

// Tests parameterized by the number of filter list catalog entries. With the
// DAT cache feature enabled, the gate providers must not trigger OnChanged on
// observers until the regional component providers have been created and
// registered. For an empty catalog, OnChanged should still fire because the
// gates are the only providers.
class AdBlockComponentServiceManagerGatesTest
    : public testing::TestWithParam<size_t> {
 protected:
  size_t CatalogSize() const { return GetParam(); }
};

TEST_P(AdBlockComponentServiceManagerGatesTest,
       GatesDoNotTriggerOnChangedUntilProvidersCreated) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kAdblockDATCache);

  TestingPrefServiceSimple prefs;
  RegisterPrefsForAdBlockService(prefs.registry());

  AdBlockFiltersProviderManager filters_provider_manager;
  AdBlockFilterListCatalogProvider catalog_provider(nullptr);
  AdBlockListP3A list_p3a(&prefs);

  AdBlockComponentServiceManager service_manager(
      &prefs, &filters_provider_manager, "en", nullptr, &catalog_provider,
      &list_p3a);

  // Pre-consume the startup change notifications so that later OnChanged calls
  // go through NotifyObservers instead of being swallowed. Without this, the
  // startup-suppression flag hides the premature gate notification we want to
  // detect.
  CountingObserver flag_consumer;
  filters_provider_manager.ForceNotifyObserver(flag_consumer,
                                               /*is_default_engine=*/true);
  filters_provider_manager.ForceNotifyObserver(flag_consumer,
                                               /*is_default_engine=*/false);

  CountingObserver observer;
  filters_provider_manager.AddObserver(&observer);

  service_manager.SetFilterListCatalog(MakeCatalog(CatalogSize()));

  const auto& default_providers =
      filters_provider_manager.GetProviders(/*is_for_default_engine=*/true);
  const auto& additional_providers =
      filters_provider_manager.GetProviders(/*is_for_default_engine=*/false);

  // Default engine: one gate + one regional provider per catalog entry (entries
  // have first_party_protections=true, so they land in the default engine).
  EXPECT_EQ(default_providers.size(), 1u + CatalogSize());
  // Additional engine only has its gate.
  EXPECT_EQ(additional_providers.size(), 1u);

  // Regional providers are un-initialized (no real component updater backs
  // them). Gates are initialized. AllProvidersInitialized is therefore only
  // true for the default engine when the catalog is empty.
  if (CatalogSize() == 0) {
    EXPECT_TRUE(AllProvidersInitialized(default_providers));
  } else {
    EXPECT_FALSE(AllProvidersInitialized(default_providers));
  }
  EXPECT_TRUE(AllProvidersInitialized(additional_providers));

  // Default engine: with non-empty catalog, gates must not fire OnChanged
  // before regional providers are registered. With empty catalog, the gate is
  // the full provider set, so OnChanged fires once.
  EXPECT_EQ(observer.default_count, CatalogSize() == 0 ? 1 : 0);
  // Additional engine always has only its gate — OnChanged fires once.
  EXPECT_EQ(observer.additional_count, 1);

  filters_provider_manager.RemoveObserver(&observer);
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdBlockComponentServiceManagerGatesTest,
                         testing::Values(0u, 1u, 3u),
                         [](const auto& info) {
                           return absl::StrFormat("CatalogSize%zu", info.param);
                         });

}  // namespace brave_shields
