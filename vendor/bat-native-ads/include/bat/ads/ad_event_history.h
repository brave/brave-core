/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_EVENT_HISTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_EVENT_HISTORY_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ads/export.h"

namespace ads {

class ADS_EXPORT AdEventHistory final {
 public:
  AdEventHistory();
  ~AdEventHistory();

  void Record(const std::string& ad_type,
              const std::string& confirmation_type,
              const double timestamp);

  std::vector<double> Get(const std::string& ad_type,
                          const std::string& confirmation_type) const;

  void Reset();

 private:
  std::map<std::string, std::vector<double>> history_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_EVENT_HISTORY_H_
