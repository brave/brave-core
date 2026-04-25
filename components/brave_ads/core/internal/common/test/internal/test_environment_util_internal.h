/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_TEST_ENVIRONMENT_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_TEST_ENVIRONMENT_UTIL_INTERNAL_H_

// Internal helpers for configuring global test environment state that is only
// needed during `TestBase` setup.

namespace brave_ads::test {

void SetUpCommandLineSwitches();

void SetUpContentSettings();

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_TEST_ENVIRONMENT_UTIL_INTERNAL_H_
