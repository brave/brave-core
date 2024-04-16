/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_creative_set_info.h"

#include <limits>

#include "base/numerics/ranges.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"

namespace brave_ads {

CatalogCreativeSetInfo::CatalogCreativeSetInfo() = default;

CatalogCreativeSetInfo::CatalogCreativeSetInfo(
    const CatalogCreativeSetInfo& other) = default;

CatalogCreativeSetInfo& CatalogCreativeSetInfo::operator=(
    const CatalogCreativeSetInfo& other) = default;

CatalogCreativeSetInfo::CatalogCreativeSetInfo(
    CatalogCreativeSetInfo&& other) noexcept = default;

CatalogCreativeSetInfo& CatalogCreativeSetInfo::operator=(
    CatalogCreativeSetInfo&& other) noexcept = default;

CatalogCreativeSetInfo::~CatalogCreativeSetInfo() = default;

bool CatalogCreativeSetInfo::operator==(
    const CatalogCreativeSetInfo& other) const {
  return id == other.id && per_day == other.per_day &&
         per_week == other.per_week && per_month == other.per_month &&
         total_max == other.total_max &&
         base::IsApproximatelyEqual(value, other.value,
                                    std::numeric_limits<double>::epsilon()) &&
         split_test_group == other.split_test_group &&
         segments == other.segments && oses == other.oses &&
         conversions == other.conversions &&
         creative_notification_ads == other.creative_notification_ads &&
         creative_inline_content_ads == other.creative_inline_content_ads &&
         creative_new_tab_page_ads == other.creative_new_tab_page_ads &&
         creative_promoted_content_ads == other.creative_promoted_content_ads;
}

bool CatalogCreativeSetInfo::operator!=(
    const CatalogCreativeSetInfo& other) const {
  return !(*this == other);
}

bool CatalogCreativeSetInfo::DoesSupportOS() const {
  if (oses.empty()) {
    // Creative set supports all operating systems
    return true;
  }

  const std::string platform_name = PlatformHelper::GetInstance().GetName();

  return base::ranges::any_of(oses, [&platform_name](const CatalogOsInfo& os) {
    return os.name == platform_name;
  });
}

}  // namespace brave_ads
