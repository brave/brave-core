/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_creative_ad_notification_info.h"

namespace ads {

CatalogCreativeAdNotificationInfo::
CatalogCreativeAdNotificationInfo() = default;

CatalogCreativeAdNotificationInfo::CatalogCreativeAdNotificationInfo(
    const CatalogCreativeAdNotificationInfo& info) = default;

CatalogCreativeAdNotificationInfo::
~CatalogCreativeAdNotificationInfo() = default;

bool CatalogCreativeAdNotificationInfo::operator==(
    const CatalogCreativeAdNotificationInfo& rhs) const {
  return payload == rhs.payload;
}

bool CatalogCreativeAdNotificationInfo::operator!=(
    const CatalogCreativeAdNotificationInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
