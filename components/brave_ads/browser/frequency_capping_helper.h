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

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

class FrequencyCappingHelper {
 public:
  FrequencyCappingHelper(const FrequencyCappingHelper&) = delete;
  FrequencyCappingHelper& operator=(const FrequencyCappingHelper&) = delete;

  FrequencyCappingHelper(FrequencyCappingHelper&& other) noexcept = delete;
  FrequencyCappingHelper& operator=(FrequencyCappingHelper&& other) noexcept =
      delete;

  static FrequencyCappingHelper* GetInstance();

  void RecordAdEventForId(const std::string& id,
                          const std::string& ad_type,
                          const std::string& confirmation_type,
                          base::Time time);

  std::vector<base::Time> GetAdEventHistory(
      const std::string& ad_type,
      const std::string& confirmation_type) const;

  void ResetAdEventHistoryForId(const std::string& id);

 private:
  friend struct base::DefaultSingletonTraits<FrequencyCappingHelper>;

  FrequencyCappingHelper();

  ~FrequencyCappingHelper();

  ads::AdEventHistory history_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_FREQUENCY_CAPPING_HELPER_H_
