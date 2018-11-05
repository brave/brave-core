/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/notification_result_info.h"

namespace ads {

NotificationResultInfo::NotificationResultInfo() :
  id(""),
  result_type(NotificationResultInfoResultType::DISMISSED),
  catalog(""),
  url(""),
  classification("") {}

NotificationResultInfo::~NotificationResultInfo() {}

}  // namespace ads
