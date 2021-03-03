/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_FTX_BROWSER_REGIONS_H_
#define BRAVE_COMPONENTS_FTX_BROWSER_REGIONS_H_

#include <string>
#include <vector>

namespace ftx {

const std::vector<std::string> unsupported_regions = {
    "AF", "AG", "CI", "CU", "IQ", "IR", "LR", "KP", "SD", "SY", "ZW"};

}  // namespace ftx

#endif  // BRAVE_COMPONENTS_FTX_BROWSER_REGIONS_H_
