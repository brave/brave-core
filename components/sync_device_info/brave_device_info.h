/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_DEVICE_INFO_BRAVE_DEVICE_INFO_H_
#define BRAVE_COMPONENTS_SYNC_DEVICE_INFO_BRAVE_DEVICE_INFO_H_

#include <optional>
#include <string>

#include "base/time/time.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync_device_info/device_info.h"

namespace base {
class DictionaryValue;
}

namespace syncer {

class BraveDeviceInfo : public DeviceInfo {
 public:
  BraveDeviceInfo(
      const std::string& guid,
      const std::string& client_name,
      const std::string& chrome_version,
      const std::string& sync_user_agent,
      const sync_pb::SyncEnums::DeviceType device_type,
      const OsType os_type,
      const FormFactor form_factor,
      const std::string& signin_scoped_device_id,
      const std::string& manufacturer_name,
      const std::string& model_name,
      const std::string& full_hardware_class,
      base::Time last_updated_timestamp,
      base::TimeDelta pulse_interval,
      bool send_tab_to_self_receiving_enabled,
      sync_pb::SyncEnums_SendTabReceivingType send_tab_to_self_receiving_type,
      const std::optional<DeviceInfo::SharingInfo>& sharing_info,
      const std::optional<PhoneAsASecurityKeyInfo>& paask_info,
      const std::string& fcm_registration_token,
      const DataTypeSet& interested_data_types,
      std::optional<base::Time> floating_workspace_last_signin_timestamp,
      bool is_self_delete_supported);
  BraveDeviceInfo(const BraveDeviceInfo&) = delete;
  BraveDeviceInfo& operator=(const BraveDeviceInfo&) = delete;
  ~BraveDeviceInfo() override {}

  bool is_self_delete_supported() const;
  void set_is_self_delete_supported(bool is_self_delete_supported);

  // Gets the OS in string form.
  std::string GetOSString() const;

  // Gets the device type in string form.
  std::string GetDeviceTypeString() const;

  // Converts the |DeviceInfo| values to a JS friendly DictionaryValue,
  // which extension APIs can expose to third party apps.
  base::Value::Dict ToValue() const;

 private:
  bool is_self_delete_supported_;
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_DEVICE_INFO_BRAVE_DEVICE_INFO_H_
