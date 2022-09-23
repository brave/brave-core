/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_utils.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveVPNUtilsUnitTest, MigrateAndMerge) {
  sync_preferences::TestingPrefServiceSyncable profile_pref_service;
  brave_vpn::RegisterProfilePrefs(profile_pref_service.registry());
  TestingPrefServiceSimple local_state_pref_service;
  brave_vpn::RegisterLocalStatePrefs(local_state_pref_service.registry());
  EXPECT_FALSE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNLocalStateMigrated));
  auto vpn_settings = base::JSONReader::Read(R"(
        {
            "device_region_name": "eu-de",
            "env": "development",
            "region_list":
            [
                {
                    "continent": "oceania",
                    "country-iso-code": "AU",
                    "name": "au-au",
                    "name-pretty": "Australia"
                }
            ]
        })");
  profile_pref_service.Set(brave_vpn::prefs::kBraveVPNRootPref, *vpn_settings);
  auto p3a_settings = base::JSONReader::Read(R"(
          {
            "days_in_month_used":
            [
                {
                    "day": 1663448400.0,
                    "value": 1.0
                }
            ],
            "first_use_time": "13307922000000000",
            "last_use_time": "13307922000000000"
          })");
  local_state_pref_service.Set(brave_vpn::prefs::kBraveVPNRootPref,
                               *p3a_settings);
  brave_vpn::MigrateVPNSettings(&profile_pref_service,
                                &local_state_pref_service);

  EXPECT_FALSE(
      profile_pref_service.HasPrefPath(brave_vpn::prefs::kBraveVPNRootPref));
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNRootPref));
  base::Value result = vpn_settings->Clone();
  auto& result_dict = result.GetDict();
  result_dict.Merge(p3a_settings->GetDict().Clone());
  EXPECT_EQ(
      local_state_pref_service.GetDict(brave_vpn::prefs::kBraveVPNRootPref),
      result);
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNLocalStateMigrated));
}

TEST(BraveVPNUtilsUnitTest, Migrate) {
  sync_preferences::TestingPrefServiceSyncable profile_pref_service;
  brave_vpn::RegisterProfilePrefs(profile_pref_service.registry());
  TestingPrefServiceSimple local_state_pref_service;
  brave_vpn::RegisterLocalStatePrefs(local_state_pref_service.registry());
  EXPECT_FALSE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNLocalStateMigrated));

  auto vpn_settings = base::JSONReader::Read(R"(
        {
            "show_button": true,
            "device_region_name": "eu-de",
            "env": "development",
            "region_list":
            [
                {
                    "continent": "oceania",
                    "country-iso-code": "AU",
                    "name": "au-au",
                    "name-pretty": "Australia"
                }
            ]
        })");
  profile_pref_service.Set(brave_vpn::prefs::kBraveVPNRootPref, *vpn_settings);
  brave_vpn::MigrateVPNSettings(&profile_pref_service,
                                &local_state_pref_service);
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNRootPref));
  vpn_settings->RemovePath("show_button");
  EXPECT_EQ(
      local_state_pref_service.GetDict(brave_vpn::prefs::kBraveVPNRootPref),
      *vpn_settings);
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNLocalStateMigrated));
}

TEST(BraveVPNUtilsUnitTest, NoMigration) {
  sync_preferences::TestingPrefServiceSyncable profile_pref_service;
  brave_vpn::RegisterProfilePrefs(profile_pref_service.registry());
  TestingPrefServiceSimple local_state_pref_service;
  brave_vpn::RegisterLocalStatePrefs(local_state_pref_service.registry());
  EXPECT_FALSE(
      profile_pref_service.HasPrefPath(brave_vpn::prefs::kBraveVPNRootPref));

  auto p3a_settings = base::JSONReader::Read(R"(
          {
            "days_in_month_used":
            [
                {
                    "day": 1663448400.0,
                    "value": 1.0
                }
            ],
            "first_use_time": "13307922000000000",
            "last_use_time": "13307922000000000"
          })");
  local_state_pref_service.Set(brave_vpn::prefs::kBraveVPNRootPref,
                               *p3a_settings);
  brave_vpn::MigrateVPNSettings(&profile_pref_service,
                                &local_state_pref_service);

  EXPECT_FALSE(
      profile_pref_service.HasPrefPath(brave_vpn::prefs::kBraveVPNRootPref));
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNRootPref));
  EXPECT_EQ(
      local_state_pref_service.GetDict(brave_vpn::prefs::kBraveVPNRootPref),
      *p3a_settings);
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNLocalStateMigrated));
}

TEST(BraveVPNUtilsUnitTest, AlreadyMigrated) {
  sync_preferences::TestingPrefServiceSyncable profile_pref_service;
  brave_vpn::RegisterProfilePrefs(profile_pref_service.registry());
  TestingPrefServiceSimple local_state_pref_service;
  brave_vpn::RegisterLocalStatePrefs(local_state_pref_service.registry());
  local_state_pref_service.SetBoolean(
      brave_vpn::prefs::kBraveVPNLocalStateMigrated, true);

  auto vpn_settings = base::JSONReader::Read(R"(
        {
            "show_button": true,
            "device_region_name": "eu-de",
            "env": "development",
            "region_list":
            [
                {
                    "continent": "oceania",
                    "country-iso-code": "AU",
                    "name": "au-au",
                    "name-pretty": "Australia"
                }
            ]
        })");
  profile_pref_service.Set(brave_vpn::prefs::kBraveVPNRootPref, *vpn_settings);
  auto p3a_settings = base::JSONReader::Read(R"(
          {
            "days_in_month_used":
            [
                {
                    "day": 1663448400.0,
                    "value": 1.0
                }
            ],
            "first_use_time": "13307922000000000",
            "last_use_time": "13307922000000000"
          })");
  local_state_pref_service.Set(brave_vpn::prefs::kBraveVPNRootPref,
                               *p3a_settings);
  brave_vpn::MigrateVPNSettings(&profile_pref_service,
                                &local_state_pref_service);
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNRootPref));
  EXPECT_EQ(
      local_state_pref_service.GetDict(brave_vpn::prefs::kBraveVPNRootPref),
      *p3a_settings);
  EXPECT_TRUE(local_state_pref_service.HasPrefPath(
      brave_vpn::prefs::kBraveVPNLocalStateMigrated));
}
