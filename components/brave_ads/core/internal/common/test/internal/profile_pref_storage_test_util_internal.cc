/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/profile_pref_storage_test_util_internal.h"

#include "base/check.h"
#include "components/prefs/testing_pref_service.h"

namespace brave_ads::test {

namespace {
TestingPrefServiceSimple* g_profile_prefs = nullptr;
}  // namespace

void SetProfilePrefServiceForTesting(TestingPrefServiceSimple& prefs) {
  g_profile_prefs = &prefs;
}

void ResetProfilePrefServiceForTesting() {
  g_profile_prefs = nullptr;
}

TestingPrefServiceSimple& GetProfilePrefServiceForTesting() {
  CHECK(g_profile_prefs) << "TestBase::SetUp has not been called";

  return *g_profile_prefs;
}

}  // namespace brave_ads::test
