/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/notification_ad/catalog_notification_ad_payload_info.h"

namespace ads {

bool CatalogNotificationAdPayloadInfo::operator==(
    const CatalogNotificationAdPayloadInfo& other) const {
  return body == other.body && title == other.title &&
         target_url == other.target_url;
}

bool CatalogNotificationAdPayloadInfo::operator!=(
    const CatalogNotificationAdPayloadInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
