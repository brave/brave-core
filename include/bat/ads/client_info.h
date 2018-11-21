/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "bat/ads/export.h"
#include "bat/ads/client_info_platform_type.h"

namespace ads {

struct ADS_EXPORT ClientInfo {
  ClientInfo();
  ClientInfo(const ClientInfo& info);
  ~ClientInfo();

  std::string application_version;

  ClientInfoPlatformType platform;
  std::string platform_version;

  bool IsMobile() const {
    if (platform == ANDROID || platform == IOS) {
      return true;
    }

    return false;
  }
};

}  // namespace ads
