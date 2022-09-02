/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync_device_info/brave_device_info.h"

#include "base/values.h"

namespace syncer {

BraveDeviceInfo::BraveDeviceInfo(
    const std::string& guid,
    const std::string& client_name,
    const std::string& chrome_version,
    const std::string& sync_user_agent,
    const sync_pb::SyncEnums::DeviceType device_type,
    const std::string& signin_scoped_device_id,
    const std::string& manufacturer_name,
    const std::string& model_name,
    base::Time last_updated_timestamp,
    base::TimeDelta pulse_interval,
    bool send_tab_to_self_receiving_enabled,
    const absl::optional<DeviceInfo::SharingInfo>& sharing_info,
    const absl::optional<PhoneAsASecurityKeyInfo>& paask_info,
    const std::string& fcm_registration_token,
    const ModelTypeSet& interested_data_types,
    bool is_self_delete_supported)
    : DeviceInfo(guid,
                 client_name,
                 chrome_version,
                 sync_user_agent,
                 device_type,
                 signin_scoped_device_id,
                 manufacturer_name,
                 model_name,
                 /*full_hardware_class=*/"",
                 last_updated_timestamp,
                 pulse_interval,
                 send_tab_to_self_receiving_enabled,
                 sharing_info,
                 paask_info,
                 fcm_registration_token,
                 interested_data_types),
      is_self_delete_supported_(is_self_delete_supported) {}

bool BraveDeviceInfo::is_self_delete_supported() const {
  return is_self_delete_supported_;
}

void BraveDeviceInfo::set_is_self_delete_supported(
    bool is_self_delete_supported) {
  is_self_delete_supported_ = is_self_delete_supported;
}

std::string BraveDeviceInfo::GetOSString() const {
  switch (device_type()) {
    case sync_pb::SyncEnums_DeviceType_TYPE_WIN:
      return "win";
    case sync_pb::SyncEnums_DeviceType_TYPE_MAC:
      return "mac";
    case sync_pb::SyncEnums_DeviceType_TYPE_LINUX:
      return "linux";
    case sync_pb::SyncEnums_DeviceType_TYPE_CROS:
      return "chrome_os";
    case sync_pb::SyncEnums_DeviceType_TYPE_PHONE:
    case sync_pb::SyncEnums_DeviceType_TYPE_TABLET:
      if (sync_user_agent().find("IOS") != std::string::npos)
        return "ios";

      return "android";
    case sync_pb::SyncEnums_DeviceType_TYPE_UNSET:
    case sync_pb::SyncEnums_DeviceType_TYPE_OTHER:
      return "unknown";
  }
}

std::string BraveDeviceInfo::GetDeviceTypeString() const {
  switch (device_type()) {
    case sync_pb::SyncEnums_DeviceType_TYPE_WIN:
    case sync_pb::SyncEnums_DeviceType_TYPE_MAC:
    case sync_pb::SyncEnums_DeviceType_TYPE_LINUX:
    case sync_pb::SyncEnums_DeviceType_TYPE_CROS:
      return "desktop_or_laptop";
    case sync_pb::SyncEnums_DeviceType_TYPE_PHONE:
      return "phone";
    case sync_pb::SyncEnums_DeviceType_TYPE_TABLET:
      return "tablet";
    case sync_pb::SyncEnums_DeviceType_TYPE_UNSET:
    case sync_pb::SyncEnums_DeviceType_TYPE_OTHER:
      return "unknown";
  }
}

base::Value::Dict BraveDeviceInfo::ToValue() const {
  base::Value::Dict dict;
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
