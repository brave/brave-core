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
  std::string GetMobileOSString(const std::string user_agent) {
    if (user_agent.find("IOS") != std::string::npos) {
	  return "ios";
	}

	if (user_agent.find("android") != std::string::npos) {
	  return "android";
	}
	return "unknown";
  }

  std::unique_ptr<base::DictionaryValue> DeviceInfo::ToValue() const {
	  std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
	  value->SetString("name", client_name_);
	  value->SetString("id", public_id_);
	  if (device_type_ == sync_pb::SyncEnums_DeviceType_TYPE_PHONE ||
	  	  device_type_ == sync_pb::SyncEnums_DeviceType_TYPE_TABLET ) {
	    value->SetString("os", GetMobileOSString(sync_user_agent_));  
	  } else {
	  	value->SetString("os", GetOSString());
	  }
	  value->SetString("type", GetDeviceTypeString());
	  value->SetString("chromeVersion", chrome_version_);
	  value->SetInteger("lastUpdatedTimestamp", last_updated_timestamp().ToTimeT());
	  value->SetBoolean("sendTabToSelfReceivingEnabled",
	                    send_tab_to_self_receiving_enabled());
	  value->SetBoolean("hasSharingInfo", sharing_info().has_value());
	  return value;
  }
}  // namespace syncer
