/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTRIBUTE_BROWSER_STATIC_VALUES_H_
#define BRAVE_COMPONENTS_CONTRIBUTE_BROWSER_STATIC_VALUES_H_

#include <string>
#include <vector>

namespace contribute {

const std::vector<std::string> kContributeBlacklistRegions = {
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

}  // namespace contribute

#endif  // BRAVE_COMPONENTS_CONTRIBUTE_BROWSER_STATIC_VALUES_H_
