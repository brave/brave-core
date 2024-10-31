/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/targeting/geographical/subdivision/supported_subdivisions.h"

#include "base/no_destructor.h"

namespace brave_ads {

namespace {

base::Value::List ToValue(const SubdivisionMap& subdivisions) {
  base::Value::List list;

  for (const auto& [subdivision, name] : subdivisions) {
    list.Append(
        base::Value::Dict().Set("subdivision", subdivision).Set("name", name));
  }

  return list;
}

}  // namespace

const SupportedSubdivisionMap& GetSupportedSubdivisions() {
  // Format: { ISO Country Code, { ISO country subdivisions as defined in ISO
  // 3166-2 https://en.wikipedia.org/wiki/ISO_3166-2, ... } }.
  static const base::NoDestructor<SupportedSubdivisionMap>
      kSupportedSubdivisions(
          {{"US",
            {// United States of America states.
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
            {// Canadian provinces.
             {"CA-AB", "Alberta"},
             {"CA-BC", "British Columbia"},
             {"CA-MB", "Manitoba"},
             {"CA-NB", "New Brunswick"},
             {"CA-NL", "Newfoundland and Labrador"},
             {"CA-NS", "Nova Scotia"},
             {"CA-NT", "Northwest Territories"},
             {"CA-NU", "Nunavut"},
             {"CA-ON", "Ontario"},
             {"CA-PE", "Prince Edward Island"},
             {"CA-QC", "Quebec"},
             {"CA-SK", "Saskatchewan"},
             {"CA-YT", "Yukon"}}}});

  return *kSupportedSubdivisions;
}

base::Value::List GetSupportedSubdivisionsAsValueList(
    const std::string& country_code) {
  const auto& supported_subdivisions = GetSupportedSubdivisions();

  const auto iter = supported_subdivisions.find(country_code);
  if (iter == supported_subdivisions.cend()) {
    return {};
  }

  const auto& [_, subdivisions] = *iter;

  return ToValue(subdivisions);
}

}  // namespace brave_ads
