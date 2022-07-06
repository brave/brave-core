/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/media_permission_rule.h"

#include "bat/ads/internal/serving/permission_rules/permission_rule_features.h"
#include "bat/ads/internal/tabs/tab_info.h"
#include "bat/ads/internal/tabs/tab_manager.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

MediaPermissionRule::MediaPermissionRule() = default;

MediaPermissionRule::~MediaPermissionRule() = default;

bool MediaPermissionRule::ShouldAllow() {
  if (!permission_rules::features::ShouldOnlyServeAdsIfMediaIsNotPlaying()) {
    return true;
  }

  if (!DoesRespectCap()) {
    last_message_ = "Media is playing";
    return false;
  }

  return true;
}

std::string MediaPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool MediaPermissionRule::DoesRespectCap() {
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance()->GetVisibleTab();
  if (!tab) {
    return true;
  }

  return !TabManager::GetInstance()->IsPlayingMedia(tab->id);
}

}  // namespace ads
