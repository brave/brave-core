/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync_device_info/device_info.h"

#include <algorithm>

#define ToValue ToValue_ChromiumImpl
#include "../../../../components/sync_device_info/device_info.cc"
#undef ToValue

namespace syncer {
std::unique_ptr<base::DictionaryValue> DeviceInfo::ToValue() const {
  std::unique_ptr<base::DictionaryValue> value = ToValue_ChromiumImpl();
  if ((device_type_ == sync_pb::SyncEnums_DeviceType_TYPE_PHONE ||
      device_type_ == sync_pb::SyncEnums_DeviceType_TYPE_TABLET) &&
      sync_user_agent_.find("IOS") != std::string::npos) {
    value->SetString("os", "ios");
  } else {
    value->SetString("os", GetOSString());
  }
  return value;
}
}  // namespace syncer
