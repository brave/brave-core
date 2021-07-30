/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/network_connection_frequency_cap.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_features.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

NetworkConnectionFrequencyCap::NetworkConnectionFrequencyCap() = default;

NetworkConnectionFrequencyCap::~NetworkConnectionFrequencyCap() = default;

bool NetworkConnectionFrequencyCap::ShouldAllow() {
  if (!features::frequency_capping::
          ShouldOnlyServeAdsWithValidInternetConnection()) {
    return true;
  }

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
  return AdsClientHelper::Get()->IsNetworkConnectionAvailable();
}

}  // namespace ads
