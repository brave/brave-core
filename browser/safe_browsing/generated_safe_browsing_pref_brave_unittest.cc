/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/values.h"
#include "brave/browser/safe_browsing/brave_generated_safe_browsing_pref.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/safebrowsing/constants.h"
#include "chrome/browser/extensions/api/settings_private/generated_pref_test_base.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "chrome/test/base/testing_profile.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace settings_private = extensions::settings_private;

namespace safe_browsing {

using BraveGeneratedSafeBrowsingPrefTest =
    settings_private::GeneratedPrefTestBase;

TEST_F(BraveGeneratedSafeBrowsingPrefTest, LimitedProtectionRoundTrip) {
  auto pref = std::make_unique<BraveGeneratedSafeBrowsingPref>(profile());

  // Seed Standard so the Limited transition is observable.
  prefs()->SetUserPref(prefs::kSafeBrowsingEnabled,
                       std::make_unique<base::Value>(true));
  prefs()->SetUserPref(prefs::kSafeBrowsingEnhanced,
                       std::make_unique<base::Value>(false));
  prefs()->SetUserPref(kBraveSafeBrowsingDownloadProtectionEnabled,
                       std::make_unique<base::Value>(true));

  ASSERT_EQ(pref->SetPref(std::make_unique<base::Value>(
                              kBraveSafeBrowsingLimitedProtection)
                              .get()),
            settings_private::SetPrefResult::SUCCESS);

  EXPECT_TRUE(prefs()->GetUserPref(prefs::kSafeBrowsingEnabled)->GetBool());
  EXPECT_FALSE(prefs()->GetUserPref(prefs::kSafeBrowsingEnhanced)->GetBool());
  EXPECT_FALSE(prefs()
                   ->GetUserPref(kBraveSafeBrowsingDownloadProtectionEnabled)
                   ->GetBool());

  EXPECT_EQ(pref->GetPrefObject().value->GetInt(),
            kBraveSafeBrowsingLimitedProtection);
}

TEST_F(BraveGeneratedSafeBrowsingPrefTest, StandardResetsLimited) {
  auto pref = std::make_unique<BraveGeneratedSafeBrowsingPref>(profile());

  ASSERT_EQ(pref->SetPref(std::make_unique<base::Value>(
                              kBraveSafeBrowsingLimitedProtection)
                              .get()),
            settings_private::SetPrefResult::SUCCESS);
  ASSERT_FALSE(prefs()
                   ->GetUserPref(kBraveSafeBrowsingDownloadProtectionEnabled)
                   ->GetBool());

  ASSERT_EQ(pref->SetPref(
                std::make_unique<base::Value>(
                    static_cast<int>(SafeBrowsingState::STANDARD_PROTECTION))
                    .get()),
            settings_private::SetPrefResult::SUCCESS);

  EXPECT_TRUE(prefs()->GetUserPref(prefs::kSafeBrowsingEnabled)->GetBool());
  EXPECT_FALSE(prefs()->GetUserPref(prefs::kSafeBrowsingEnhanced)->GetBool());
  EXPECT_TRUE(prefs()
                  ->GetUserPref(kBraveSafeBrowsingDownloadProtectionEnabled)
                  ->GetBool());
  EXPECT_EQ(pref->GetPrefObject().value->GetInt(),
            static_cast<int>(SafeBrowsingState::STANDARD_PROTECTION));
}

TEST_F(BraveGeneratedSafeBrowsingPrefTest, GetPrefObjectReflectsBackingPrefs) {
  auto pref = std::make_unique<BraveGeneratedSafeBrowsingPref>(profile());

  prefs()->SetUserPref(prefs::kSafeBrowsingEnabled,
                       std::make_unique<base::Value>(true));
  prefs()->SetUserPref(prefs::kSafeBrowsingEnhanced,
                       std::make_unique<base::Value>(false));
  prefs()->SetUserPref(kBraveSafeBrowsingDownloadProtectionEnabled,
                       std::make_unique<base::Value>(false));

  EXPECT_EQ(pref->GetPrefObject().value->GetInt(),
            kBraveSafeBrowsingLimitedProtection);

  // Re-enable download protection -> Standard.
  prefs()->SetUserPref(kBraveSafeBrowsingDownloadProtectionEnabled,
                       std::make_unique<base::Value>(true));
  EXPECT_EQ(pref->GetPrefObject().value->GetInt(),
            static_cast<int>(SafeBrowsingState::STANDARD_PROTECTION));

  // Turn SB off entirely -> No protection, independent of the Brave pref.
  prefs()->SetUserPref(prefs::kSafeBrowsingEnabled,
                       std::make_unique<base::Value>(false));
  prefs()->SetUserPref(kBraveSafeBrowsingDownloadProtectionEnabled,
                       std::make_unique<base::Value>(false));
  EXPECT_EQ(pref->GetPrefObject().value->GetInt(),
            static_cast<int>(SafeBrowsingState::NO_SAFE_BROWSING));
}

TEST_F(BraveGeneratedSafeBrowsingPrefTest,
       LimitedRejectedWhenSafeBrowsingEnforced) {
  auto pref = std::make_unique<BraveGeneratedSafeBrowsingPref>(profile());

  prefs()->SetManagedPref(prefs::kSafeBrowsingEnabled,
                          std::make_unique<base::Value>(true));

  EXPECT_EQ(pref->SetPref(std::make_unique<base::Value>(
                              kBraveSafeBrowsingLimitedProtection)
                              .get()),
            settings_private::SetPrefResult::PREF_NOT_MODIFIABLE);

  // Rejected write must leave the backing pref alone. GetBoolean (defaulted
  // true) avoids depending on a prior user write.
  EXPECT_TRUE(prefs()->GetBoolean(kBraveSafeBrowsingDownloadProtectionEnabled));
}

}  // namespace safe_browsing
