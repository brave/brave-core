/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLIENT_PREFERENCES_SAVED_AD_INFO_H_
#define BAT_ADS_INTERNAL_CLIENT_PREFERENCES_SAVED_AD_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/result.h"

namespace ads {

struct SavedAdInfo {
  SavedAdInfo();
  SavedAdInfo(
      const SavedAdInfo& info);
  ~SavedAdInfo();

  std::string ToJson() const;
  Result FromJson(
      const std::string& json);

  std::string creative_instance_id;
  std::string creative_set_id;
};

using SavedAdList = std::vector<SavedAdInfo>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLIENT_PREFERENCES_SAVED_AD_INFO_H_
