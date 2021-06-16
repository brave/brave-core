/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/media_frequency_cap.h"

#include "bat/ads/internal/frequency_capping/frequency_capping_features.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/tab_manager/tab_manager.h"

namespace ads {

MediaFrequencyCap::MediaFrequencyCap() = default;

MediaFrequencyCap::~MediaFrequencyCap() = default;

bool MediaFrequencyCap::ShouldAllow() {
  if (!features::frequency_capping::ShouldOnlyServeAdsIfMediaIsNotPlaying()) {
    return true;
  }

  if (!DoesRespectCap()) {
    last_message_ = "Media is playing";
    return false;
  }

  return true;
}

std::string MediaFrequencyCap::get_last_message() const {
  return last_message_;
}

bool MediaFrequencyCap::DoesRespectCap() {
  const base::Optional<TabInfo> tab = TabManager::Get()->GetVisible();
  if (!tab) {
    return true;
  }

  return !TabManager::Get()->IsPlayingMedia(tab->id);
}

}  // namespace ads
