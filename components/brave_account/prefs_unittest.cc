/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/prefs.h"

#include <optional>
#include <string>

#include "base/check_deref.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::prefs {

namespace {

struct MigrateObsoleteProfilePrefsTestCase {
  std::string test_name;
  // The dict to seed before migrating, or std::nullopt to leave the registered
  // default ({ "kind": "logged-out" }) in place.
  std::optional<base::DictValue> seed;
  // The `kind` expected after migrating.
  const char* expected_kind;
};

// A profile that wrote the pref before the default existed has an empty dict
// (no `kind`). The migration stamps { "kind": "logged-out" } onto it.
const MigrateObsoleteProfilePrefsTestCase* StampsKindOntoLegacyEmptyDict() {
  static base::NoDestructor<MigrateObsoleteProfilePrefsTestCase> test_case({
      .test_name = "stamps_kind_onto_legacy_empty_dict",
      .seed = base::DictValue(),
      .expected_kind = state_kinds::kLoggedOut,
  });
  return test_case.get();
}

// An already-valid state (here, logged-in) must be left untouched.
const MigrateObsoleteProfilePrefsTestCase* LeavesExistingKindUntouched() {
  static base::NoDestructor<MigrateObsoleteProfilePrefsTestCase> test_case({
      .test_name = "leaves_existing_kind_untouched",
      .seed = base::DictValue().Set(keys::kKind, state_kinds::kLoggedIn),
      .expected_kind = state_kinds::kLoggedIn,
  });
  return test_case.get();
}

// A freshly-registered pref already carries the default `kind`; the migration
// is a no-op.
const MigrateObsoleteProfilePrefsTestCase* NoOpOnDefaultDict() {
  static base::NoDestructor<MigrateObsoleteProfilePrefsTestCase> test_case({
      .test_name = "no_op_on_default_dict",
      .seed = std::nullopt,
      .expected_kind = state_kinds::kLoggedOut,
  });
  return test_case.get();
}

using BraveAccountPrefsMigrationTest =
    testing::TestWithParam<const MigrateObsoleteProfilePrefsTestCase*>;

}  // namespace

TEST_P(BraveAccountPrefsMigrationTest, MigrateObsoleteProfilePrefs) {
  const auto& test_case = CHECK_DEREF(GetParam());

  TestingPrefServiceSimple pref_service;
  RegisterPrefs(pref_service.registry());

  if (test_case.seed) {
    pref_service.SetDict(kBraveAccountState, test_case.seed->Clone());
  }

  MigrateObsoleteProfilePrefs(&pref_service);

  const auto* kind =
      pref_service.GetDict(kBraveAccountState).FindString(keys::kKind);
  ASSERT_TRUE(kind);
  EXPECT_EQ(*kind, test_case.expected_kind);
}

INSTANTIATE_TEST_SUITE_P(BraveAccountPrefsTests,
                         BraveAccountPrefsMigrationTest,
                         testing::Values(StampsKindOntoLegacyEmptyDict(),
                                         LeavesExistingKindUntouched(),
                                         NoOpOnDefaultDict()),
                         [](const auto& info) {
                           return info.param->test_name;
                         });

}  // namespace brave_account::prefs
