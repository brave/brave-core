/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_PROMOTED_CONTENT_AD_INFO_H_
#define BAT_ADS_PROMOTED_CONTENT_AD_INFO_H_

#include <string>

#include "bat/ads/ad_info.h"
#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT PromotedContentAdInfo : AdInfo {
  PromotedContentAdInfo();
  PromotedContentAdInfo(
      const PromotedContentAdInfo& info);
  ~PromotedContentAdInfo();

  bool IsValid() const;

  std::string ToJson() const;
  Result FromJson(
      const std::string& json);

  std::string title;
  std::string description;
};

}  // namespace ads

#endif  // BAT_ADS_PROMOTED_CONTENT_AD_INFO_H_
