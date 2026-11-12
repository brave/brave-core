/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_INFO_H_

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"

namespace brave_ads {

struct CampaignConversionInfo final {
  CampaignConversionInfo();

  CampaignConversionInfo(const CampaignConversionInfo&);
  CampaignConversionInfo& operator=(const CampaignConversionInfo&);

  CampaignConversionInfo(CampaignConversionInfo&&) noexcept;
  CampaignConversionInfo& operator=(CampaignConversionInfo&&) noexcept;

  ~CampaignConversionInfo();

  bool operator==(const CampaignConversionInfo&) const = default;

  [[nodiscard]] bool IsValid() const;

  std::string id;

  // Matches an earlier checkout page to read from, since the page matching
  // `thank_you_url_pattern` does not always still show the order value.
  std::optional<std::string> checkout_url_pattern;

  std::string thank_you_url_pattern;

  bool extract_id = false;
  bool extract_value = false;

  base::TimeDelta observation_window;
  std::optional<base::Time> expire_at;
};

using CampaignConversionList = std::vector<CampaignConversionInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_INFO_H_
