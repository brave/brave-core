/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MOONPAY_BROWSER_REGIONS_H_
#define BRAVE_COMPONENTS_MOONPAY_BROWSER_REGIONS_H_

#include <string>
#include <vector>

namespace moonpay {

const std::vector<std::string> bitcoin_dot_com_supported_regions = {
  "GB", "AT", "BE", "BG", "HR",
  "CY", "CZ", "DK", "EE", "FI",
  "FR", "DE", "GR", "HU", "IE",
  "IT", "LV", "LT", "LU", "MT",
  "NL", "PL", "PT", "RO", "SK",
  "SI", "ES", "SE", "CA", "AU",
  "RU", "CH", "ZA", "NZ", "US"
};

}  // namespace moonpay

#endif  // BRAVE_COMPONENTS_MOONPAY_BROWSER_REGIONS_H_
