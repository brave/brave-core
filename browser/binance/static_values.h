/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BINANCE_STATIC_VALUES_H_
#define BRAVE_BROWSER_BINANCE_STATIC_VALUES_H_

#include <string>
#include <vector>

namespace binance {

const std::vector<std::string> kBinanceBlacklistRegions = {
  "BY",
  "BI",
  "CF",
  "CD",
  "UA",
  "CU",
  "IR",
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

#endif  // BRAVE_BROWSER_BINANCE_STATIC_VALUES_H_
