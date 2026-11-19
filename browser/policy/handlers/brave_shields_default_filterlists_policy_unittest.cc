/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// Test that the BraveShieldsDefaultFilterlists managed policy pref
// correctly enables the specified filter list UUIDs in
// AdBlockComponentServiceManager::IsFilterListEnabled().
class BraveShieldsDefaultFilterlistsPolicyTest : public testing::Test {
 protected:
  void SetUp() override {
    registry()->RegisterDictionaryPref(
        brave_shields::prefs::kAdBlockRegionalFilters);
    registry()->RegisterListPref(kManagedBraveShieldsDefaultFilterlists);
    registry()->RegisterBooleanPref(
        brave_shields::prefs::kAdBlockCheckedDefaultRegion, false);
    registry()->RegisterBooleanPref(
        brave_shields::prefs::kAdBlockCheckedAllDefaultRegions, false);
    registry()->RegisterBooleanPref(
        brave_shields::prefs::kAdBlockOnlyModeEnabled, false);
    registry()->RegisterBooleanPref(
        brave_shields::prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale,
        false);
  }

  PrefRegistrySimple* registry() { return pref_service_.registry(); }
  TestingPrefServiceSimple pref_service_;

  // A sample UUID from the adblock-resources catalog.
  static constexpr char kTestUuid[] = "FBpkI776-E-vub7x";
  static constexpr char kOtherUuid[] = "2F3DCE16-A19A-493C-A88F-2E110FBD37D2";
};

// When no policy is set, the managed pref should not influence the list.
TEST_F(BraveShieldsDefaultFilterlistsPolicyTest,
       NoPolicyDoesNotAffectManagedPref) {
  // The managed pref is not set by policy (empty list by default).
  EXPECT_FALSE(
      pref_service_.IsManagedPreference(kManagedBraveShieldsDefaultFilterlists));
}

// When the policy is set with a UUID, the managed pref should contain it.
TEST_F(BraveShieldsDefaultFilterlistsPolicyTest,
       PolicySetsCorrectManagedPrefValue) {
  base::Value::List list;
  list.Append(kTestUuid);
  pref_service_.SetManagedPref(kManagedBraveShieldsDefaultFilterlists,
                               base::Value(std::move(list)));

  EXPECT_TRUE(
      pref_service_.IsManagedPreference(kManagedBraveShieldsDefaultFilterlists));

  const auto& managed_list =
      pref_service_.GetList(kManagedBraveShieldsDefaultFilterlists);
  ASSERT_EQ(managed_list.size(), 1u);
  EXPECT_EQ(managed_list[0].GetString(), kTestUuid);
}

// When the policy contains multiple UUIDs, all should appear in the pref.
TEST_F(BraveShieldsDefaultFilterlistsPolicyTest,
       PolicyWithMultipleUuids) {
  base::Value::List list;
  list.Append(kTestUuid);
  list.Append(kOtherUuid);
  pref_service_.SetManagedPref(kManagedBraveShieldsDefaultFilterlists,
                               base::Value(std::move(list)));

  const auto& managed_list =
      pref_service_.GetList(kManagedBraveShieldsDefaultFilterlists);
  ASSERT_EQ(managed_list.size(), 2u);
  EXPECT_EQ(managed_list[0].GetString(), kTestUuid);
  EXPECT_EQ(managed_list[1].GetString(), kOtherUuid);
}

// Setting an empty list via policy results in an empty managed pref.
TEST_F(BraveShieldsDefaultFilterlistsPolicyTest,
       PolicyWithEmptyList) {
  pref_service_.SetManagedPref(kManagedBraveShieldsDefaultFilterlists,
                               base::Value(base::Value::List()));

  EXPECT_TRUE(
      pref_service_.IsManagedPreference(kManagedBraveShieldsDefaultFilterlists));
  EXPECT_TRUE(
      pref_service_.GetList(kManagedBraveShieldsDefaultFilterlists).empty());
}
