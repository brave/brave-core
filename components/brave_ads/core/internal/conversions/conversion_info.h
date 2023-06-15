/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"

namespace brave_ads {

struct ConversionInfo final {
  ConversionInfo();

  ConversionInfo(const ConversionInfo&);
  ConversionInfo& operator=(const ConversionInfo&);

  ConversionInfo(ConversionInfo&&) noexcept;
  ConversionInfo& operator=(ConversionInfo&&) noexcept;

  ~ConversionInfo();

  [[nodiscard]] bool IsValid() const;

  bool operator==(const ConversionInfo&) const;
  bool operator!=(const ConversionInfo&) const;

  std::string creative_set_id;
  std::string type;
  std::string url_pattern;
  std::string advertiser_public_key;
  base::TimeDelta observation_window;
  base::Time expire_at;
};

using ConversionList = std::vector<ConversionInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_INFO_H_
