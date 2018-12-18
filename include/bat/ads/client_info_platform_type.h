/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CLIENT_INFO_PLATFORM_TYPE_H_
#define BAT_ADS_CLIENT_INFO_PLATFORM_TYPE_H_

#include "bat/ads/export.h"

namespace ads {

enum ADS_EXPORT ClientInfoPlatformType {
  UNKNOWN,
  WIN7,
  WIN8,
  WIN10,
  MACOS,
  IOS,
  ANDROID_OS,
  LINUX
};

}  // namespace ads

#endif  // BAT_ADS_CLIENT_INFO_PLATFORM_TYPE_H_
