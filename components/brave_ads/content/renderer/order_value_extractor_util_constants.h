/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_EXTRACTOR_UTIL_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_EXTRACTOR_UTIL_CONSTANTS_H_

#include <string_view>

namespace brave_ads {

// A regex pattern that matches a bare number, including optional grouping
// separators and decimal places.
inline constexpr std::string_view kBareNumberPattern =
    R"RE2(((?:\d{1,3}(?:[.,\p{Zs}]\d{3})+|\d+)(?:[.,]\d{2})?))RE2";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_EXTRACTOR_UTIL_CONSTANTS_H_
