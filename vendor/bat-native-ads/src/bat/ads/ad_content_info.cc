/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_content_info.h"

namespace ads {

AdContentInfo::AdContentInfo() = default;

AdContentInfo::AdContentInfo(const AdContentInfo& other) = default;

AdContentInfo& AdContentInfo::operator=(const AdContentInfo& other) = default;

AdContentInfo::AdContentInfo(AdContentInfo&& other) noexcept = default;

AdContentInfo& AdContentInfo::operator=(AdContentInfo&& other) noexcept =
    default;

AdContentInfo::~AdContentInfo() = default;

AdContentLikeActionType AdContentInfo::ToggleThumbUpActionType() const {
  if (like_action_type == AdContentLikeActionType::kThumbsUp) {
    return AdContentLikeActionType::kNeutral;
  }

  return AdContentLikeActionType::kThumbsUp;
}

AdContentLikeActionType AdContentInfo::ToggleThumbDownActionType() const {
  if (like_action_type == AdContentLikeActionType::kThumbsDown) {
    return AdContentLikeActionType::kNeutral;
  }

  return AdContentLikeActionType::kThumbsDown;
}

bool operator==(const AdContentInfo& lhs, const AdContentInfo& rhs) {
  return lhs.type == rhs.type && lhs.placement_id == rhs.placement_id &&
         lhs.creative_instance_id == rhs.creative_instance_id &&
         lhs.creative_set_id == rhs.creative_set_id &&
         lhs.campaign_id == rhs.campaign_id &&
         lhs.advertiser_id == rhs.advertiser_id && lhs.brand == rhs.brand &&
         lhs.brand_info == rhs.brand_info &&
         lhs.brand_display_url == rhs.brand_display_url &&
         lhs.brand_url == rhs.brand_url &&
         lhs.like_action_type == rhs.like_action_type &&
         lhs.confirmation_type == rhs.confirmation_type &&
         lhs.is_saved == rhs.is_saved && lhs.is_flagged == rhs.is_flagged;
}

bool operator!=(const AdContentInfo& lhs, const AdContentInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
