/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/media_permission_rule.h"

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

bool DoesRespectCap() {
  if (!kShouldOnlyServeAdsIfMediaIsNotPlaying.Get()) {
    return true;
  }

  const absl::optional<TabInfo> tab = TabManager::GetInstance().GetVisible();
  if (!tab) {
    return true;
  }

  return !TabManager::GetInstance().IsPlayingMedia(tab->id);
}

}  // namespace

base::expected<void, std::string> MediaPermissionRule::ShouldAllow() const {
  if (!DoesRespectCap()) {
    return base::unexpected("Media is playing");
  }

  return base::ok();
}

}  // namespace brave_ads
