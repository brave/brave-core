/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/browser_is_active_frequency_cap.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/platform/platform_helper.h"

namespace ads {

BrowserIsActiveFrequencyCap::BrowserIsActiveFrequencyCap(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

BrowserIsActiveFrequencyCap::~BrowserIsActiveFrequencyCap() = default;

bool BrowserIsActiveFrequencyCap::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "Browser window is not active";
    return false;
  }

  return true;
}

std::string BrowserIsActiveFrequencyCap::get_last_message() const {
  return last_message_;
}

bool BrowserIsActiveFrequencyCap::DoesRespectCap() {
  if (PlatformHelper::GetInstance()->GetPlatform() == PlatformType::kAndroid) {
    return true;
  }

  if (!ads_->IsForeground()) {
    return false;
  }

  return true;
}

}  // namespace ads
