/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/notification_ad/catalog_creative_notification_ad_info.h"

namespace ads {

CatalogCreativeNotificationAdInfo::CatalogCreativeNotificationAdInfo() =
    default;

CatalogCreativeNotificationAdInfo::CatalogCreativeNotificationAdInfo(
    const CatalogCreativeNotificationAdInfo& info) = default;

CatalogCreativeNotificationAdInfo& CatalogCreativeNotificationAdInfo::operator=(
    const CatalogCreativeNotificationAdInfo& info) = default;

CatalogCreativeNotificationAdInfo::CatalogCreativeNotificationAdInfo(
    CatalogCreativeNotificationAdInfo&& other) noexcept = default;

CatalogCreativeNotificationAdInfo& CatalogCreativeNotificationAdInfo::operator=(
    CatalogCreativeNotificationAdInfo&& other) noexcept = default;

CatalogCreativeNotificationAdInfo::~CatalogCreativeNotificationAdInfo() =
    default;

bool CatalogCreativeNotificationAdInfo::operator==(
    const CatalogCreativeNotificationAdInfo& other) const {
  return CatalogCreativeInfo::operator==(other) && payload == other.payload;
}

bool CatalogCreativeNotificationAdInfo::operator!=(
    const CatalogCreativeNotificationAdInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
