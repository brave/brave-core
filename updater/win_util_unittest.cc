/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/util/win_util.h"

#include <optional>
#include <string>

#include "base/strings/sys_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/test_reg_util_win.h"
#include "base/win/registry.h"
#include "chrome/updater/registration_data.h"
#include "chrome/updater/updater_scope.h"
#include "chrome/updater/util/util.h"
#include "chrome/updater/win/win_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace updater {

namespace {

void CreateClientsKey(const std::wstring& app_id) {
  base::win::RegKey clients_key;
  ASSERT_EQ(clients_key.Create(HKEY_CURRENT_USER,
                               GetAppClientsKey(app_id).c_str(), KEY_WRITE),
            ERROR_SUCCESS);
  // MigrateLegacyUpdaters skips apps without a valid `pv`, so we must set it.
  ASSERT_EQ(clients_key.WriteValue(kRegValuePV, L"1.2.3.4"), ERROR_SUCCESS);
}

void SetApValue(const std::wstring& app_id, const std::wstring& ap) {
  base::win::RegKey state_key;
  ASSERT_EQ(state_key.Create(HKEY_CURRENT_USER,
                             GetAppClientStateKey(app_id).c_str(), KEY_WRITE),
            ERROR_SUCCESS);
  ASSERT_EQ(state_key.WriteValue(kRegValueAP, ap.c_str()), ERROR_SUCCESS);
}

void SetCohort(const std::wstring& app_id, const std::wstring& cohort) {
  base::win::RegKey cohort_key;
  ASSERT_EQ(cohort_key.Create(HKEY_CURRENT_USER,
                              GetAppCohortKey(app_id).c_str(), KEY_WRITE),
            ERROR_SUCCESS);
  ASSERT_EQ(cohort_key.WriteValue(nullptr, cohort.c_str()), ERROR_SUCCESS);
}

// Registers a legacy app with the given `ap` and explicit cohort (either can
// be nullptr for "not present"), runs MigrateLegacyUpdaters and returns the
// cohort it reports for the app. Uses EXPECT_* instead of ASSERT_* because
// gtest does not support ASSERT_* in functions with a return value.
std::optional<std::string> MigrateAndGetCohort(const wchar_t* ap,
                                               const wchar_t* explicit_cohort) {
  const std::wstring app_id = L"test-app";
  registry_util::RegistryOverrideManager registry_override;
  EXPECT_NO_FATAL_FAILURE(
      registry_override.OverrideRegistry(HKEY_CURRENT_USER));
  EXPECT_NO_FATAL_FAILURE(CreateClientsKey(app_id));
  if (ap) {
    EXPECT_NO_FATAL_FAILURE(SetApValue(app_id, ap));
  }
  if (explicit_cohort) {
    EXPECT_NO_FATAL_FAILURE(SetCohort(app_id, explicit_cohort));
  }

  bool registered = false;
  std::optional<std::string> cohort;
  EXPECT_TRUE(MigrateLegacyUpdaters(
      UpdaterScope::kUser,
      base::BindLambdaForTesting([&](const RegistrationRequest& registration) {
        if (registration.app_id == base::SysWideToUTF8(app_id)) {
          registered = true;
          cohort = registration.cohort;
        }
      })));
  EXPECT_TRUE(registered) << "ap: " << (ap ? ap : L"<not set>");
  return cohort;
}

}  // namespace

TEST(WinUtil, MigrateLegacyUpdatersAssignsCohort) {
  // Universal channels:
  EXPECT_EQ(MigrateAndGetCohort(L"release", nullptr), std::nullopt);
  EXPECT_EQ(MigrateAndGetCohort(L"release-test", nullptr), "private");
  EXPECT_EQ(MigrateAndGetCohort(L"nightly", nullptr), std::nullopt);
  EXPECT_EQ(MigrateAndGetCohort(L"nightly-test", nullptr), "private");

  // Legacy channels encoded the architecture:
  EXPECT_EQ(MigrateAndGetCohort(L"64-be", nullptr), std::nullopt);
  EXPECT_EQ(MigrateAndGetCohort(L"64-be-test", nullptr), "private");

  // Failed Omaha 3 delta updates append "-full" to the channel:
  EXPECT_EQ(MigrateAndGetCohort(L"release-full", nullptr), std::nullopt);
  EXPECT_EQ(MigrateAndGetCohort(L"release-test-full", nullptr), "private");

  // No `ap` value:
  EXPECT_EQ(MigrateAndGetCohort(nullptr, nullptr), std::nullopt);

  // An explicit cohort takes precedence over `ap`:
  EXPECT_EQ(MigrateAndGetCohort(L"release-test", L"explicit-cohort"),
            "explicit-cohort");
}

}  // namespace updater
