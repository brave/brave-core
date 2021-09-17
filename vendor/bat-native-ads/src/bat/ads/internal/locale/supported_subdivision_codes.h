/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_SUBDIVISION_CODES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_SUBDIVISION_CODES_H_

#include "bat/ads/internal/locale/supported_subdivision_codes_aliases.h"

namespace ads {

const SupportedSubdivisionCodesMap kSupportedSubdivisionCodes = {
    //  Format: { ISO Country Code, { ISO country subdivision codes as defined
    //  in
    //  ISO 3166-2 https://en.wikipedia.org/wiki/ISO_3166-2, ... } }
    {"US",
     {
         // United States of America
         "US-AL",  // Alabama
         "US-AK",  // Alaska
         "US-AZ",  // Arizona
         "US-AR",  // Arkansas
         "US-CA",  // California
         "US-CO",  // Colorado
         "US-CT",  // Connecticut
         "US-DE",  // Delaware
         "US-FL",  // Florida
         "US-GA",  // Georgia
         "US-HI",  // Hawaii
         "US-ID",  // Idaho
         "US-IL",  // Illinois
         "US-IN",  // Indiana
         "US-IA",  // Iowa
         "US-KS",  // Kansas
         "US-KY",  // Kentucky
         "US-LA",  // Louisiana
         "US-ME",  // Maine
         "US-MD",  // Maryland
         "US-MA",  // Massachusetts
         "US-MI",  // Michigan
         "US-MN",  // Minnesota
         "US-MS",  // Mississippi
         "US-MO",  // Missouri
         "US-MT",  // Montana
         "US-NE",  // Nebraska
         "US-NV",  // Nevada
         "US-NH",  // New Hampshire
         "US-NJ",  // New Jersey
         "US-NM",  // New Mexico
         "US-NY",  // New York
         "US-NC",  // North Carolina
         "US-ND",  // North Dakota
         "US-OH",  // Ohio
         "US-OK",  // Oklahoma
         "US-OR",  // Oregon
         "US-PA",  // Pennsylvania
         "US-RI",  // Rhode Island
         "US-SC",  // South Carolina
         "US-SD",  // South Dakota
         "US-TN",  // Tennessee
         "US-TX",  // Texas
         "US-UT",  // Utah
         "US-VT",  // Vermont
         "US-VA",  // Virginia
         "US-WA",  // Washington
         "US-WV",  // West Virginia
         "US-WI",  // Wisconsin
         "US-WY",  // Wyoming
     }}};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_SUBDIVISION_CODES_H_
