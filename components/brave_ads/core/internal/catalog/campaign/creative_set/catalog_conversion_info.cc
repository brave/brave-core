/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_conversion_info.h"

#include <tuple>

namespace brave_ads {

CatalogConversionInfo::CatalogConversionInfo() = default;

CatalogConversionInfo::CatalogConversionInfo(
    const CatalogConversionInfo& other) = default;

CatalogConversionInfo& CatalogConversionInfo::operator=(
    const CatalogConversionInfo& other) = default;

CatalogConversionInfo::CatalogConversionInfo(
    CatalogConversionInfo&& other) noexcept = default;

CatalogConversionInfo& CatalogConversionInfo::operator=(
    CatalogConversionInfo&& other) noexcept = default;

CatalogConversionInfo::~CatalogConversionInfo() = default;

bool operator==(const CatalogConversionInfo& lhs,
                const CatalogConversionInfo& rhs) {
  const auto tie = [](const CatalogConversionInfo& conversion) {
    return std::tie(conversion.creative_set_id, conversion.url_pattern,
                    conversion.verifiable_advertiser_public_key_base64,
                    conversion.observation_window, conversion.expire_at);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CatalogConversionInfo& lhs,
                const CatalogConversionInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
