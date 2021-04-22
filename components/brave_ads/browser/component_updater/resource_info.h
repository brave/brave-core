/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_INFO_H_

#include <stdint.h>

#include <string>

#include "base/files/file_path.h"

namespace brave_ads {

struct ResourceInfo {
  std::string id;
  uint16_t version;
  base::FilePath path;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_RESOURCE_INFO_H_
