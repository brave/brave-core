/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CLIENT_INFO_H_
#define BAT_ADS_CLIENT_INFO_H_

#include <string>

#include "bat/ads/export.h"
#include "bat/ads/client_info_platform_type.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT ClientInfo {
  ClientInfo();
  ClientInfo(
      const ClientInfo& info);
  ~ClientInfo();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  ClientInfoPlatformType platform = ClientInfoPlatformType::UNKNOWN;
};

}  // namespace ads

#endif  // BAT_ADS_CLIENT_INFO_H_
