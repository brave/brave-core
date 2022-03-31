/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_CONTENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_CONTENT_INFO_H_

#include <string>

#include "bat/ads/ad_content_action_types.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/export.h"

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace ads {

struct ADS_EXPORT AdContentInfo final {
  AdContentInfo();
  AdContentInfo(const AdContentInfo& info);
  ~AdContentInfo();

  bool operator==(const AdContentInfo& rhs) const;
  bool operator!=(const AdContentInfo& rhs) const;

  AdContentLikeActionType ToggleThumbUpActionType() const;
  AdContentLikeActionType ToggleThumbDownActionType() const;

  base::DictionaryValue ToValue() const;
  bool FromValue(const base::Value& value);

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  AdType type = AdType::kUndefined;
  std::string uuid;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  std::string brand;
  std::string brand_info;
  std::string brand_display_url;
  std::string brand_url;
  AdContentLikeActionType like_action_type = AdContentLikeActionType::kNeutral;
  ConfirmationType confirmation_type = ConfirmationType::kUndefined;
  bool is_saved = false;
  bool is_flagged = false;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_CONTENT_INFO_H_
