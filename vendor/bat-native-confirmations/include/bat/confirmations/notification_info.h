/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_NOTIFICATION_INFO_H_
#define BAT_CONFIRMATIONS_NOTIFICATION_INFO_H_

#include <string>

#include "bat/confirmations/export.h"
#include "bat/confirmations/confirmation_type.h"

namespace confirmations {

struct CONFIRMATIONS_EXPORT NotificationInfo {
  NotificationInfo();
  explicit NotificationInfo(const NotificationInfo& info);
  ~NotificationInfo();

  std::string id;
  std::string creative_set_id;
  std::string category;
  std::string advertiser;
  std::string text;
  std::string url;
  std::string uuid;
  ConfirmationType type;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_NOTIFICATION_INFO_H_
