/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SAVED_AD_H_
#define BAT_ADS_INTERNAL_SAVED_AD_H_

#include <string>

#include "bat/ads/result.h"

namespace ads {

struct SavedAd {
  SavedAd();
  SavedAd(
      const SavedAd& ad);
  ~SavedAd();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::string uuid;
  std::string creative_set_id;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SAVED_AD_H_
