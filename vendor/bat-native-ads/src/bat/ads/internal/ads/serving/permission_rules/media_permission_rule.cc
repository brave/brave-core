/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/media_permission_rule.h"

#include "absl/types/optional.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "bat/ads/internal/tabs/tab_manager.h"

namespace ads {

namespace {

bool DoesRespectCap() {
  const absl::optional<TabInfo> tab = TabManager::GetInstance()->GetVisible();
  if (!tab) {
    return true;
  }

  return !TabManager::GetInstance()->IsPlayingMedia(tab->id);
}

}  // namespace

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

const std::string& MediaPermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
