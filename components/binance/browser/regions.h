/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BINANCE_BROWSER_REGIONS_H_
#define BRAVE_COMPONENTS_BINANCE_BROWSER_REGIONS_H_

#include <string>
#include <vector>

namespace binance {

const std::vector<std::string> unsupported_regions = {
  "BY",
  "BI",
  "CF",
  "CD",
  "UA",
  "CU",
  "IR",
  "JP",
  "LB",
  "LY",
  "KP",
  "RU",
  "SO",
  "SS",
  "SD",
  "SY",
  "VE",
  "YE",
  "ZW"
};

}  // namespace binance

#endif  // BRAVE_COMPONENTS_BINANCE_BROWSER_REGIONS_H_
