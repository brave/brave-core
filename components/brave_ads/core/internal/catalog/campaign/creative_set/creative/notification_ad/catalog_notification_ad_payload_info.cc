/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/notification_ad/catalog_notification_ad_payload_info.h"

#include <tuple>

namespace brave_ads {

bool CatalogNotificationAdPayloadInfo::operator==(
    const CatalogNotificationAdPayloadInfo& other) const {
  const auto tie = [](const CatalogNotificationAdPayloadInfo& payload) {
    return std::tie(payload.body, payload.title, payload.target_url);
  };

  return tie(*this) == tie(other);
}

bool CatalogNotificationAdPayloadInfo::operator!=(
    const CatalogNotificationAdPayloadInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads
