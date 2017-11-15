/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_UTIL_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_UTIL_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "content/public/common/resource_type.h"

class AdBlockClient;

namespace brave_shields {

bool GetDATFileData(const std::string& fileName,
  std::vector<unsigned char>& buffer);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_UTIL_
