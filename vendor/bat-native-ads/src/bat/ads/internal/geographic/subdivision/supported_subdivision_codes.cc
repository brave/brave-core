/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/geographic/subdivision/supported_subdivision_codes.h"

#include "base/no_destructor.h"
#include "base/strings/string_piece.h"

namespace ads::geographic {

const SupportedSubdivisionCodesMap& GetSupportedSubdivisionCodes() {
  //  Format: { ISO Country Code, { ISO country subdivision codes as defined
  //  in ISO 3166-2 https://en.wikipedia.org/wiki/ISO_3166-2, ... } }
  static const base::NoDestructor<SupportedSubdivisionCodesMap>
      kSupportedSubdivisionCodes(
          {{"US",
            {// United States of America
             {"US-AL", "Alabama"},        {"US-AK", "Alaska"},
             {"US-AZ", "Arizona"},        {"US-AR", "Arkansas"},
             {"US-CA", "California"},     {"US-CO", "Colorado"},
             {"US-CT", "Connecticut"},    {"US-DE", "Delaware"},
             {"US-FL", "Florida"},        {"US-GA", "Georgia"},
             {"US-HI", "Hawaii"},         {"US-ID", "Idaho"},
             {"US-IL", "Illinois"},       {"US-IN", "Indiana"},
             {"US-IA", "Iowa"},           {"US-KS", "Kansas"},
             {"US-KY", "Kentucky"},       {"US-LA", "Louisiana"},
             {"US-ME", "Maine"},          {"US-MD", "Maryland"},
             {"US-MA", "Massachusetts"},  {"US-MI", "Michigan"},
             {"US-MN", "Minnesota"},      {"US-MS", "Mississippi"},
             {"US-MO", "Missouri"},       {"US-MT", "Montana"},
             {"US-NE", "Nebraska"},       {"US-NV", "Nevada"},
             {"US-NH", "New Hampshire"},  {"US-NJ", "New Jersey"},
             {"US-NM", "New Mexico"},     {"US-NY", "New York"},
             {"US-NC", "North Carolina"}, {"US-ND", "North Dakota"},
             {"US-OH", "Ohio"},           {"US-OK", "Oklahoma"},
             {"US-OR", "Oregon"},         {"US-PA", "Pennsylvania"},
             {"US-RI", "Rhode Island"},   {"US-SC", "South Carolina"},
             {"US-SD", "South Dakota"},   {"US-TN", "Tennessee"},
             {"US-TX", "Texas"},          {"US-UT", "Utah"},
             {"US-VT", "Vermont"},        {"US-VA", "Virginia"},
             {"US-WA", "Washington"},     {"US-WV", "West Virginia"},
             {"US-WI", "Wisconsin"},      {"US-WY", "Wyoming"}}},
           {"CA",
            {// Canada
             {"CA-AB", "Alberta"},
             {"CA-BC", "British Columbia"},
             {"CA-MB", "Manitoba"},
             {"CA-NB", "New Brunswick"},
             {"CA-NS", "Nova Scotia"},
             {"CA-ON", "Ontario"},
             {"CA-QC", "Quebec"},
             {"CA-SK", "Saskatchewan"}}}});

  return *kSupportedSubdivisionCodes;
}

bool IsSupportedSubdivisionCode(const std::string& country_code,
                                const std::string& subdivision_code) {
  const auto iter = GetSupportedSubdivisionCodes().find(country_code);
  if (iter == GetSupportedSubdivisionCodes().cend()) {
    return false;
  }

  const auto& subdivisions = iter->second;
  return subdivisions.find(subdivision_code) != subdivisions.cend();
}

}  // namespace ads::geographic
