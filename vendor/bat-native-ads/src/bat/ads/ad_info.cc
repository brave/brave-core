/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_info.h"

namespace ads {

AdInfo::AdInfo() = default;

AdInfo::AdInfo(
    const AdInfo& info) = default;

AdInfo::~AdInfo() = default;

bool AdInfo::IsValid() const {
  if (type == AdType::kUndefined ||
      creative_instance_id.empty() ||
      creative_set_id.empty() ||
      campaign_id.empty() ||
      segment.empty() ||
      target_url.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads
