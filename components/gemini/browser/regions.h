/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GEMINI_BROWSER_REGIONS_H_
#define BRAVE_COMPONENTS_GEMINI_BROWSER_REGIONS_H_

#include <string>
#include <vector>

namespace gemini {

const std::vector<std::string> supported_regions = {
  // Canada
  "CA",
  // Great Britain
  "GB",
  // United States
  "US",
  // EU Countries
  "BE", "BG", "CZ", "DK", "DE",
  "EE", "IE", "EL", "ES", "FR",
  "HR", "IT", "CY", "LV", "LT",
  "LU", "HU", "MT", "NL", "AT",
  "PL", "PT", "RO", "SI", "SK",
  "FI", "SE"
};

}  // namespace gemini

#endif  // BRAVE_COMPONENTS_GEMINI_BROWSER_REGIONS_H_
