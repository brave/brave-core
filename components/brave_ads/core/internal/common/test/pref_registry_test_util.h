/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_PREF_REGISTRY_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_PREF_REGISTRY_TEST_UTIL_H_

namespace brave_ads::test {

// TODO(https://github.com/brave/brave-browser/issues/34528): Refactor unit test
// pref mocks to use PrefService.

void RegisterLocalStatePrefs();
void RegisterProfilePrefs();

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_PREF_REGISTRY_TEST_UTIL_H_
