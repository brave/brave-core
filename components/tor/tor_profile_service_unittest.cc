/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_profile_service.h"

#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tor {

TEST(TorProfileServiceTest, MigrateObsoleteProfilePrefs) {
  constexpr char kObsoleteOnionOnlyInTorWindows[] =
      "tor.onion_only_in_tor_windows";

  TestingPrefServiceSimple prefs;
  TorProfileService::RegisterProfilePrefsForMigration(prefs.registry());
  prefs.SetBoolean(kObsoleteOnionOnlyInTorWindows, false);
  ASSERT_TRUE(prefs.HasPrefPath(kObsoleteOnionOnlyInTorWindows));

  TorProfileService::MigrateObsoleteProfilePrefs(&prefs);

  EXPECT_FALSE(prefs.HasPrefPath(kObsoleteOnionOnlyInTorWindows));
}

}  // namespace tor
