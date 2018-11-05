/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "bat/ads/export.h"

namespace ads {

enum ADS_EXPORT NotificationResultInfoResultType {
  CLICKED,
  DISMISSED,
  TIMEOUT
};

struct ADS_EXPORT NotificationResultInfo {
  NotificationResultInfo();
  ~NotificationResultInfo();

  std::string id;
  NotificationResultInfoResultType result_type;
  std::string catalog;
  std::string url;
  std::string classification;
};

}  // namespace ads
