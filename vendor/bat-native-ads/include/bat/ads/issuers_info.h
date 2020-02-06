/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ISSUERS_INFO_H_
#define BAT_ADS_ISSUERS_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/export.h"
#include "bat/ads/issuer_info.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT IssuersInfo {
  IssuersInfo();
  IssuersInfo(
      const IssuersInfo& info);
  ~IssuersInfo();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::string public_key;
  std::vector<IssuerInfo> issuers;
};

}  // namespace ads

#endif  // BAT_ADS_ISSUERS_INFO_H_
