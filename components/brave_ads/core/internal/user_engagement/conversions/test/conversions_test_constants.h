/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_TEST_CONVERSIONS_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_TEST_CONVERSIONS_TEST_CONSTANTS_H_

namespace brave_ads::test {

// Default conversion.
inline constexpr char kMatchingUrlPattern[] = "https://foo.com/*";
inline constexpr char kAnotherMatchingUrlPattern[] = "https://qux.com/*/corge";
inline constexpr char kMismatchingUrlPattern[] =
    "https://www.grault.com/garply";

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_TEST_CONVERSIONS_TEST_CONSTANTS_H_
