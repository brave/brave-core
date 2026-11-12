/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_RESOURCE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_RESOURCE_CONSTANTS_H_

namespace brave_ads {

// A `resources.json` manifest key, not a Component Updater component ID
// (mirrors `kAntiTargetingResourceId`). Resolves to `conversion-config.json`,
// shipped in the same existing resources component as `campaigns.json` and
// `sites.json`, not a dedicated component of its own.
inline constexpr char kOrderExtractionPatternsResourceId[] =
    "conversion-config";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_RESOURCE_CONSTANTS_H_
