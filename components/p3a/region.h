/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_REGION_H_
#define BRAVE_COMPONENTS_P3A_REGION_H_

#include <string_view>

namespace p3a {

struct RegionIdentifiers {
  std::string_view region;
  std::string_view sub_region;
};

RegionIdentifiers GetRegionIdentifiers(std::string_view country_code);

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_REGION_H_
