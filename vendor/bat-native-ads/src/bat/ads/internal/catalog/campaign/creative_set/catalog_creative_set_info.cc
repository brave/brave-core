/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/catalog_creative_set_info.h"

#include "base/ranges/algorithm.h"
#include "bat/ads/internal/base/numbers/number_util.h"
#include "bat/ads/internal/base/platform/platform_helper.h"

namespace ads {

CatalogCreativeSetInfo::CatalogCreativeSetInfo() = default;

CatalogCreativeSetInfo::CatalogCreativeSetInfo(
    const CatalogCreativeSetInfo& info) = default;

CatalogCreativeSetInfo& CatalogCreativeSetInfo::operator=(
    const CatalogCreativeSetInfo& info) = default;

CatalogCreativeSetInfo::~CatalogCreativeSetInfo() = default;

bool CatalogCreativeSetInfo::operator==(
    const CatalogCreativeSetInfo& rhs) const {
  return creative_set_id == rhs.creative_set_id && per_day == rhs.per_day &&
         per_week == rhs.per_week && per_month == rhs.per_month &&
         total_max == rhs.total_max && DoubleEquals(value, rhs.value) &&
         split_test_group == rhs.split_test_group && segments == rhs.segments &&
         oses == rhs.oses &&
         creative_notification_ads == rhs.creative_notification_ads &&
         creative_inline_content_ads == rhs.creative_inline_content_ads &&
         creative_new_tab_page_ads == rhs.creative_new_tab_page_ads &&
         creative_promoted_content_ads == rhs.creative_promoted_content_ads &&
         conversions == rhs.conversions;
}

bool CatalogCreativeSetInfo::operator!=(
    const CatalogCreativeSetInfo& rhs) const {
  return !(*this == rhs);
}

bool CatalogCreativeSetInfo::DoesSupportOS() const {
  if (oses.empty()) {
    // Creative set supports all operating systems
    return true;
  }

  const std::string platform_name = PlatformHelper::GetInstance()->GetName();

  return base::ranges::any_of(oses, [&platform_name](const CatalogOsInfo& os) {
    return os.name == platform_name;
  });
}

}  // namespace ads
