/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/frequency_capping_helper.h"

namespace brave_ads {

FrequencyCappingHelper::FrequencyCappingHelper() = default;

FrequencyCappingHelper::~FrequencyCappingHelper() = default;

FrequencyCappingHelper* FrequencyCappingHelper::GetInstance() {
  return base::Singleton<FrequencyCappingHelper>::get();
}

void FrequencyCappingHelper::RecordAdEvent(const std::string& ad_type,
                                           const std::string& confirmation_type,
                                           const uint64_t timestamp) {
  ad_event_history_.Record(ad_type, confirmation_type, timestamp);
}

std::vector<uint64_t> FrequencyCappingHelper::GetAdEvents(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  return ad_event_history_.Get(ad_type, confirmation_type);
}

void FrequencyCappingHelper::ResetAdEvents() {
  ad_event_history_.Reset();
}

}  // namespace brave_ads
