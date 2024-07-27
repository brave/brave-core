/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/media_permission_rule.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"

namespace brave_ads {

bool HasMediaPermission() {
  if (!kShouldOnlyServeAdsIfMediaIsNotPlaying.Get()) {
    return true;
  }

  const std::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetVisible();
  if (!tab) {
    return true;
  }

  if (!TabManager::GetInstance().IsPlayingMedia(tab->id)) {
    return true;
  }

  BLOG(2, "Media is playing");
  return false;
}

}  // namespace brave_ads
