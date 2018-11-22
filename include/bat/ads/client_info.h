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

  const std::string GetPlatformName() const {
    std::string platform_name = "";

    switch(platform) {
      case UNKNOWN: {
        break;
      }
      case WIN7: {
        platform_name = "Win7";
        break;
      }
      case WIN10: {
        platform_name = "Win10";
        break;
      }
      case MACOS: {
        platform_name = "OSX";
        break;
      }
      case IOS: {
        platform_name = "iOS";
        break;
      }
      case ANDROID: {
        platform_name = "Android";
        break;
      }
      case LINUX: {
        platform_name = "Linux";
        break;
      }
    }

    return platform_name;
  }
};

}  // namespace ads
