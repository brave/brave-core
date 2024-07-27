/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_TEST_CONSTANTS_H_

namespace brave_ads::test {

// See `test/data/resources/nnqccijfhvzwyrxpxwjrpmynaiazctqb` for conversion
// resource test data.

// Also, see `conversions_test_util.h` for test helpers to construct redirect
// chains and URL patterns.

// Default conversion.
inline constexpr char kMatchingUrlPattern[] = "https://foo.com/*";
inline constexpr char kAnotherMatchingUrlPattern[] = "https://qux.com/*/corge";
inline constexpr char kMismatchingUrlPattern[] =
    "https://www.grault.com/garply";

// Verifiable conversion.
inline constexpr char kMatchingVerifiableConversionUrlPattern[] =
    "https://foo.com/bar?qux_id=xyz*";  // Matches xyzzy.
inline constexpr char kMismatchingVerifiableConversionUrlPattern[] =
    "https://foo.com/bar?qux_id=thud";

inline constexpr char kVerifiableConversionHtml[] =
    R"(<html>Hello World!<div id="xyzzy-id">waldo</div><meta name="ad-conversion-id" content="fred"></html>)";

inline constexpr char kVerifiableConversionIdPattern[] = "qux_id=(.*)";

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_TEST_CONSTANTS_H_
