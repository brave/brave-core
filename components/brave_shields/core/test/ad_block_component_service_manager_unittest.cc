// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"

#include <memory>
#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/json/json_writer.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

constexpr auto kFilterListCatalogEntries =
    base::MakeFixedFlatMap<std::string_view /*uuid*/, bool /*default_enabled*/>(
        {
            {kDefaultAdblockFiltersListUuid, true},
            {kFirstPartyAdblockFiltersListUuid, true},
            {kAdblockOnlySupplementalListUuid, false},
            {kCookieListUuid, true},
        });

}  // namespace

class AdBlockComponentServiceManagerTest : public testing::Test {
 public:
  AdBlockComponentServiceManagerTest() = default;
  ~AdBlockComponentServiceManagerTest() override = default;

  void SetUp() override {
    PrefRegistrySimple* registry = local_state_.registry();
    registry->RegisterBooleanPref(prefs::kAdBlockOnlyModeEnabled, false);
    registry->RegisterBooleanPref(prefs::kAdBlockCheckedDefaultRegion, false);
    registry->RegisterBooleanPref(prefs::kAdBlockCheckedAllDefaultRegions,
                                  false);
    registry->RegisterDictionaryPref(prefs::kAdBlockRegionalFilters);
  }

  void SetUpAdBlockComponentServiceManager() {
    list_p3a_ = std::make_unique<AdBlockListP3A>(&local_state_);
    catalog_provider_ =
        std::make_unique<AdBlockFilterListCatalogProvider>(nullptr);
    manager_ = std::make_unique<AdBlockComponentServiceManager>(
        &local_state_, /*filters_provider_manager=*/nullptr, "en-US",
        /*cus=*/nullptr, catalog_provider_.get(), list_p3a_.get());
    base::ListValue catalog;
    for (const auto& [uuid, default_enabled] : kFilterListCatalogEntries) {
      catalog.Append(base::DictValue()
                         .Set("uuid", uuid)
                         .Set("default_enabled", default_enabled));
    }
    manager_->OnFilterListCatalogLoaded(
        *base::WriteJson(base::Value(std::move(catalog))));
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<AdBlockListP3A> list_p3a_;
  std::unique_ptr<AdBlockFilterListCatalogProvider> catalog_provider_;
  std::unique_ptr<AdBlockComponentServiceManager> manager_;
};

TEST_F(AdBlockComponentServiceManagerTest,
       ABOMFilterListsAtCatalogDefaultsWhenABOMPrefDisabled) {
  feature_list_.InitAndEnableFeature(features::kAdblockOnlyMode);
  SetUpAdBlockComponentServiceManager();

  EXPECT_TRUE(manager_->IsFilterListEnabled(kDefaultAdblockFiltersListUuid));
  EXPECT_TRUE(manager_->IsFilterListEnabled(kFirstPartyAdblockFiltersListUuid));
  EXPECT_FALSE(manager_->IsFilterListEnabled(kAdblockOnlySupplementalListUuid));
  EXPECT_TRUE(manager_->IsFilterListEnabled(kCookieListUuid));
}

TEST_F(AdBlockComponentServiceManagerTest,
       FilterListsMatchExpectedStateAtInitWhenABOMPrefEnabled) {
  feature_list_.InitAndEnableFeature(features::kAdblockOnlyMode);
  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  SetUpAdBlockComponentServiceManager();

  EXPECT_TRUE(manager_->IsFilterListEnabled(kDefaultAdblockFiltersListUuid));
  EXPECT_TRUE(manager_->IsFilterListEnabled(kFirstPartyAdblockFiltersListUuid));
  EXPECT_TRUE(manager_->IsFilterListEnabled(kAdblockOnlySupplementalListUuid));

  EXPECT_FALSE(manager_->IsFilterListEnabled(kCookieListUuid));
}

TEST_F(AdBlockComponentServiceManagerTest,
       EnablingABOMPrefSetsFilterListsToExpectedState) {
  feature_list_.InitAndEnableFeature(features::kAdblockOnlyMode);
  SetUpAdBlockComponentServiceManager();

  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);

  EXPECT_TRUE(manager_->IsFilterListEnabled(kDefaultAdblockFiltersListUuid));
  EXPECT_TRUE(manager_->IsFilterListEnabled(kFirstPartyAdblockFiltersListUuid));
  EXPECT_TRUE(manager_->IsFilterListEnabled(kAdblockOnlySupplementalListUuid));

  EXPECT_FALSE(manager_->IsFilterListEnabled(kCookieListUuid));
}

TEST_F(AdBlockComponentServiceManagerTest,
       ABOMFilterListsAtCatalogDefaultsWhenABOMFeatureDisabled) {
  feature_list_.InitAndDisableFeature(features::kAdblockOnlyMode);
  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  SetUpAdBlockComponentServiceManager();

  EXPECT_TRUE(manager_->IsFilterListEnabled(kDefaultAdblockFiltersListUuid));
  EXPECT_TRUE(manager_->IsFilterListEnabled(kFirstPartyAdblockFiltersListUuid));
  EXPECT_FALSE(manager_->IsFilterListEnabled(kAdblockOnlySupplementalListUuid));
  EXPECT_TRUE(manager_->IsFilterListEnabled(kCookieListUuid));
}

}  // namespace brave_shields
