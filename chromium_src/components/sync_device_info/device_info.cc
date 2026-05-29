// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/values.h"

#include <components/sync_device_info/device_info.cc>

namespace syncer {

std::string DeviceInfo::GetOSString() const {
  switch (os_type()) {
    case OsType::kUnknown:
      return "unknown";
    case OsType::kWindows:
      return "win";
    case OsType::kMac:
      return "mac";
    case OsType::kLinux:
      return "linux";
    case OsType::kChromeOsAsh:
    case OsType::kChromeOsLacros:
      return "chrome_os";
    case OsType::kAndroid:
      return "android";
    case OsType::kIOS:
      return "ios";
    case OsType::kFuchsia:
      return "fuchisa";
  }
}

std::string DeviceInfo::GetDeviceTypeString() const {
  switch (form_factor()) {
    case FormFactor::kUnknown:
      return "unknown";
    case FormFactor::kDesktop:
      return "desktop_or_laptop";
    case FormFactor::kPhone:
      return "phone";
    case FormFactor::kTablet:
      return "tablet";
    case FormFactor::kAutomotive:
      return "tablet";
    case FormFactor::kWearable:
      return "tablet";
    case FormFactor::kTv:
      return "tablet";
  }
}

base::DictValue DeviceInfo::ToValue() const {
  base::DictValue dict;
  dict.Set("name", client_name());
  dict.Set("id", public_id());
  dict.Set("os", GetOSString());
  dict.Set("type", GetDeviceTypeString());
  dict.Set("chromeVersion", chrome_version());
  dict.Set("lastUpdatedTimestamp",
           static_cast<int>(last_updated_timestamp().ToTimeT()));
  dict.Set("sendTabToSelfReceivingEnabled",
           send_tab_to_self_receiving_enabled());
  dict.Set("hasSharingInfo", sharing_info().has_value());
  return dict;
}

}  // namespace syncer
