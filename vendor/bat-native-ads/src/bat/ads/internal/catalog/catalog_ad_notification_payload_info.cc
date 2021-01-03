/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_ad_notification_payload_info.h"

namespace ads {

CatalogAdNotificationPayloadInfo::CatalogAdNotificationPayloadInfo() = default;

CatalogAdNotificationPayloadInfo::CatalogAdNotificationPayloadInfo(
    const CatalogAdNotificationPayloadInfo& info) = default;

CatalogAdNotificationPayloadInfo::~CatalogAdNotificationPayloadInfo() = default;

bool CatalogAdNotificationPayloadInfo::operator==(
    const CatalogAdNotificationPayloadInfo& rhs) const {
  return body == rhs.body &&
      title == rhs.title &&
      target_url == rhs.target_url;
}

bool CatalogAdNotificationPayloadInfo::operator!=(
    const CatalogAdNotificationPayloadInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
