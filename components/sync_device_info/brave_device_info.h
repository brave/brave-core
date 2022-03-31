/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_DEVICE_INFO_BRAVE_DEVICE_INFO_H_
#define BRAVE_COMPONENTS_SYNC_DEVICE_INFO_BRAVE_DEVICE_INFO_H_

#include <string>

#include "base/time/time.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync_device_info/device_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace syncer {

class BraveDeviceInfo : public DeviceInfo {
 public:
  BraveDeviceInfo(const std::string& guid,
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
                  bool is_self_delete_supported);
  BraveDeviceInfo(const BraveDeviceInfo&) = delete;
  BraveDeviceInfo& operator=(const BraveDeviceInfo&) = delete;
  ~BraveDeviceInfo() override {}

  bool is_self_delete_supported() const;
  void set_is_self_delete_supported(bool is_self_delete_supported);

 private:
  bool is_self_delete_supported_;
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_DEVICE_INFO_BRAVE_DEVICE_INFO_H_
