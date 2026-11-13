/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_TEST_ORDER_EXTRACTION_PATTERNS_RESOURCE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_TEST_ORDER_EXTRACTION_PATTERNS_RESOURCE_TEST_UTIL_H_

#include <string_view>

namespace brave_ads {

struct OrderExtractionPatternsInfo;

namespace test {

inline constexpr std::string_view kWaldoExampleDomain = "waldo.example";
inline constexpr std::string_view kXyzzyExampleDomain = "xyzzy.example";
inline constexpr std::string_view kGraultExampleDomain = "grault.example";
inline constexpr std::string_view kFooExampleDomain = "foo.example";
inline constexpr std::string_view kBarExampleDomain = "bar.example";
inline constexpr std::string_view kGarplyBazExampleDomain =
    "garply.baz.example";
inline constexpr std::string_view kQuxExampleDomain = "qux.example";
inline constexpr std::string_view kQuuxExampleDomain = "quux.example";
inline constexpr std::string_view kCorgeExampleDomain = "corge.example";

OrderExtractionPatternsInfo ReadConversionConfigFromResource();

OrderExtractionPatternsInfo BuildExpectedConversionConfig();

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_TEST_ORDER_EXTRACTION_PATTERNS_RESOURCE_TEST_UTIL_H_
