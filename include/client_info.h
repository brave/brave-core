/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "export.h"

namespace ads {

ADS_EXPORT struct ClientInfo {
  std::string application_version;

  std::string platform;
  std::string platform_version;
};

}  // namespace ads
