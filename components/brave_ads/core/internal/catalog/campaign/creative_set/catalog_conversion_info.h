/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CONVERSION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CONVERSION_INFO_H_

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"

namespace brave_ads {

struct CatalogConversionInfo final {
  CatalogConversionInfo();

  CatalogConversionInfo(const CatalogConversionInfo&);
  CatalogConversionInfo& operator=(const CatalogConversionInfo&);

  CatalogConversionInfo(CatalogConversionInfo&&) noexcept;
  CatalogConversionInfo& operator=(CatalogConversionInfo&&) noexcept;

  ~CatalogConversionInfo();

  bool operator==(const CatalogConversionInfo&) const = default;

  std::string creative_set_id;

  // Matches an earlier checkout page to read from, since the page matching
  // `thank_you_url_pattern` does not always still show the order value.
  std::optional<std::string> checkout_url_pattern;

  std::string thank_you_url_pattern;

  bool extract_id = false;
  bool extract_value = false;

  base::TimeDelta observation_window;
  base::Time expire_at;
};

using CatalogConversionList = std::vector<CatalogConversionInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CONVERSION_INFO_H_
