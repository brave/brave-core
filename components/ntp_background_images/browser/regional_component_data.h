/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_REGIONAL_COMPONENT_DATA_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_REGIONAL_COMPONENT_DATA_H_

#include <string>

#include "base/optional.h"

struct RegionalComponentData {
  std::string region;
  std::string component_base64_public_key;
  std::string component_id;
};

base::Optional<RegionalComponentData> GetRegionalComponentData(
    const std::string& region);

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_REGIONAL_COMPONENT_DATA_H_
