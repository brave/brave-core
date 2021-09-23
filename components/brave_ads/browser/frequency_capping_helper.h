/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_FREQUENCY_CAPPING_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_FREQUENCY_CAPPING_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/singleton.h"
#include "bat/ads/ad_event_history.h"

namespace brave_ads {

class FrequencyCappingHelper {
 public:
  static FrequencyCappingHelper* GetInstance();

  void RecordAdEvent(const std::string& ad_type,
                     const std::string& confirmation_type,
                     const double timestamp);

  std::vector<double> GetAdEvents(const std::string& ad_type,
                                  const std::string& confirmation_type) const;

  void ResetAdEvents();

 private:
  friend struct base::DefaultSingletonTraits<FrequencyCappingHelper>;

  FrequencyCappingHelper();
  ~FrequencyCappingHelper();

  ads::AdEventHistory history_;

  FrequencyCappingHelper(const FrequencyCappingHelper&) = delete;
  FrequencyCappingHelper& operator=(const FrequencyCappingHelper&) = delete;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_FREQUENCY_CAPPING_HELPER_H_
