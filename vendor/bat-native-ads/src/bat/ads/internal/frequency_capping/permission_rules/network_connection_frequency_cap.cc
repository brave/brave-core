/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/network_connection_frequency_cap.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

NetworkConnectionFrequencyCap::NetworkConnectionFrequencyCap(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

NetworkConnectionFrequencyCap::~NetworkConnectionFrequencyCap() = default;

bool NetworkConnectionFrequencyCap::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "Network connection is unavailable";
    return false;
  }

  return true;
}

std::string NetworkConnectionFrequencyCap::get_last_message() const {
  return last_message_;
}

bool NetworkConnectionFrequencyCap::DoesRespectCap() {
  return ads_->get_ads_client()->IsNetworkConnectionAvailable();
}

}  // namespace ads
