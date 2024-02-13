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
  std::string url_pattern;
  std::optional<std::string> verifiable_advertiser_public_key_base64;
  base::TimeDelta observation_window;
  base::Time expire_at;
};

using CatalogConversionList = std::vector<CatalogConversionInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CONVERSION_INFO_H_
