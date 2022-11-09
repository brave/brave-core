/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/ntp_background_prefs.h"

#include "brave/components/constants/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

class NTPBackgroundPrefsTest : public testing::Test {
 public:
  NTPBackgroundPrefsTest() {
    NTPBackgroundPrefs::RegisterPref(service_.registry());
  }

  sync_preferences::TestingPrefServiceSyncable service_;
  NTPBackgroundPrefs background_prefs_{&service_};
};

TEST_F(NTPBackgroundPrefsTest, RegisterDefaultPref) {
  const auto& dict = service_.GetDict(NTPBackgroundPrefs::kPrefName);
  EXPECT_TRUE(dict.FindString("type"));
  EXPECT_TRUE(dict.FindBool("random").has_value());
  EXPECT_TRUE(dict.FindString("selected_value"));
}

TEST_F(NTPBackgroundPrefsTest, TypeAccessor) {
  EXPECT_TRUE(background_prefs_.IsBraveType());

  background_prefs_.SetType(NTPBackgroundPrefs::Type::kCustomImage);
  EXPECT_TRUE(background_prefs_.IsCustomImageType());

  background_prefs_.SetType(NTPBackgroundPrefs::Type::kColor);
  EXPECT_TRUE(background_prefs_.IsColorType());
}

TEST_F(NTPBackgroundPrefsTest, MigrationTest) {
  auto* registry = service_.registry();
  registry->RegisterBooleanPref(NTPBackgroundPrefs::kDeprecatedPrefName, false);
  EXPECT_FALSE(service_.GetBoolean(NTPBackgroundPrefs::kDeprecatedPrefName));

  // Check default value
  EXPECT_TRUE(background_prefs_.IsBraveType());

  // Check if migration does nothing when custom background was not enabled.
  background_prefs_.MigrateOldPref();
  EXPECT_TRUE(background_prefs_.IsBraveType());

  // Check if migration works properly.
  service_.SetBoolean(NTPBackgroundPrefs::kDeprecatedPrefName, true);
  background_prefs_.MigrateOldPref();
  EXPECT_TRUE(background_prefs_.IsCustomImageType());
  EXPECT_FALSE(service_.GetBoolean(NTPBackgroundPrefs::kDeprecatedPrefName));
}

TEST_F(NTPBackgroundPrefsTest, SelectedValue) {
  EXPECT_TRUE(background_prefs_.IsBraveType());

  constexpr char kSelectedURL[] = "http://selected.com/img.jpg";
  background_prefs_.SetSelectedValue(kSelectedURL);
  auto selected_value = background_prefs_.GetSelectedValue();
  EXPECT_TRUE(absl::holds_alternative<GURL>(selected_value));
  EXPECT_TRUE(absl::get<GURL>(selected_value).spec() == kSelectedURL);

  background_prefs_.SetType(NTPBackgroundPrefs::Type::kCustomImage);
  selected_value = background_prefs_.GetSelectedValue();
  EXPECT_TRUE(absl::holds_alternative<std::string>(selected_value));

  background_prefs_.SetType(NTPBackgroundPrefs::Type::kColor);
  background_prefs_.SetSelectedValue("red");
  selected_value = background_prefs_.GetSelectedValue();
  EXPECT_TRUE(absl::holds_alternative<std::string>(selected_value));
  EXPECT_TRUE(absl::get<std::string>(selected_value) == "red");
}
