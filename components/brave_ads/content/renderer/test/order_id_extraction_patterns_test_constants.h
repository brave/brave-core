/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_ORDER_ID_EXTRACTION_PATTERNS_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_ORDER_ID_EXTRACTION_PATTERNS_TEST_CONSTANTS_H_

#include <string_view>

// Mirrors the real `conversions.order.id.regex` patterns delivered on the
// Sponsored Images Component; keep these in sync with the real patterns
// whenever they change, so drift between the two cannot hide a real
// regression from these tests.

namespace brave_ads::test {

inline constexpr std::string_view kDeOrderIdRegex =
    R"RE2((?i)^[\s\p{Zs}]*(?:(?:Deine|Ihre|Ihr|Dein|Unsere)[\s\p{Zs}]+)?(?:Bestell|Auftrags|Buchungs|Bestätigungs|Reservierungs)(?:nummer|[\s\p{Zs}-]*nr\.?|[\s\p{Zs}-]*ID)[\s\p{Zs}]*[:#.]?[\s\p{Zs}]*#?[\s\p{Zs}]*([A-Za-z0-9][A-Za-z0-9_/-]{5,39})[\s\p{Zs}]*$)RE2";

inline constexpr std::string_view kEnOrderIdRegex =
    R"RE2((?i)^[\s\p{Zs}]*(?:(?:Your|The)[\s\p{Zs}]+)?(?:Order|Booking|Reservation|Confirmation)(?:[\s\p{Zs}]+(?:Number|No\.?|ID|Reference|Ref\.?|Code)|[\s\p{Zs}]*#)[\s\p{Zs}]*[:#.]?[\s\p{Zs}]*#?[\s\p{Zs}]*([A-Za-z0-9][A-Za-z0-9_/-]{5,39})[\s\p{Zs}]*$)RE2";

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_ORDER_ID_EXTRACTION_PATTERNS_TEST_CONSTANTS_H_
