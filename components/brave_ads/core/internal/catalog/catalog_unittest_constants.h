/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_UNITTEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_UNITTEST_CONSTANTS_H_

namespace brave_ads {

inline constexpr char kCatalogFilename[] = "catalog.json";
inline constexpr char kEmptyCatalogFilename[] = "empty_catalog.json";
inline constexpr char kCatalogWithSingleCampaignFilename[] =
    "catalog_with_single_campaign.json";
inline constexpr char kCatalogWithMultipleCampaignsFilename[] =
    "catalog_with_multiple_campaigns.json";

inline constexpr char kInvalidCatalogJson[] = "INVALID_JSON";

inline constexpr char kCatalogId[] = "29e5c8bc0ba319069980bb390d8e8f9b58c05a20";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_UNITTEST_CONSTANTS_H_
