/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_LOCAL_STATE_PREF_STORAGE_TEST_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_LOCAL_STATE_PREF_STORAGE_TEST_UTIL_INTERNAL_H_

class TestingPrefServiceSimple;

namespace brave_ads::test {

// `TestBase` calls this once per test to point pref-related test utilities at
// the real `PrefService` backing the current test's simulated local state.
void SetLocalStatePrefServiceForTesting(TestingPrefServiceSimple& prefs);

// `TestBase` calls this from `TearDown` so the pointer above does not dangle
// after the `PrefService` it refers to is destroyed.
void ResetLocalStatePrefServiceForTesting();

TestingPrefServiceSimple& GetLocalStatePrefServiceForTesting();

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_LOCAL_STATE_PREF_STORAGE_TEST_UTIL_INTERNAL_H_
