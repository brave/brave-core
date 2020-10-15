/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/ad_exclusion_rules/new_tab_page_ad_wallpaper_frequency_cap.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {
const int kWallpaperCap = 1;
}  // namespace

NewTabPageAdWallpaperFrequencyCap::NewTabPageAdWallpaperFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

NewTabPageAdWallpaperFrequencyCap::
~NewTabPageAdWallpaperFrequencyCap() = default;

bool NewTabPageAdWallpaperFrequencyCap::ShouldExclude(
    const AdInfo& ad) {
  const std::map<std::string, std::deque<uint64_t>>& history =
      ads_->get_client()->GetNewTabPageAdHistory();

  const std::deque<uint64_t> filtered_history = FilterHistory(history, ad.uuid);

  if (!DoesRespectCap(filtered_history, ad)) {
    last_message_ = base::StringPrintf("uuid %s has exceeded the "
        "frequency capping for new tab page ad", ad.uuid.c_str());
    return true;
  }

  return false;
}

std::string NewTabPageAdWallpaperFrequencyCap::get_last_message() const {
  return last_message_;
}

bool NewTabPageAdWallpaperFrequencyCap::DoesRespectCap(
    const std::deque<uint64_t>& history,
    const AdInfo& ad) {
  if (history.size() >= kWallpaperCap) {
    return false;
  }

  return true;
}

std::deque<uint64_t> NewTabPageAdWallpaperFrequencyCap::FilterHistory(
    const std::map<std::string, std::deque<uint64_t>>& history,
    const std::string& uuid) {
  std::deque<uint64_t> filtered_history;

  if (history.find(uuid) != history.end()) {
    filtered_history = history.at(uuid);
  }

  return filtered_history;
}

}  // namespace ads
