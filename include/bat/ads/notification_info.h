/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT NotificationInfo {
  NotificationInfo();
  NotificationInfo(const NotificationInfo& info);

  ~NotificationInfo();

  std::string category;
  std::string advertiser;
  std::string text;
  std::string url;
  std::string uuid;
};

}  // namespace ads
