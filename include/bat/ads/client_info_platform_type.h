/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "bat/ads/export.h"

namespace ads {

enum ADS_EXPORT ClientInfoPlatformType {
  UNKNOWN,
  WIN7,
  WIN10,
  OSX,
  IOS,
  ANDROID,
  LINUX
};

}  // namespace ads
