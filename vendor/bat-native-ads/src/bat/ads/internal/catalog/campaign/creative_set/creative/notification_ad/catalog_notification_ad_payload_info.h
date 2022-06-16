/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NOTIFICATION_AD_CATALOG_NOTIFICATION_AD_PAYLOAD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NOTIFICATION_AD_CATALOG_NOTIFICATION_AD_PAYLOAD_INFO_H_

#include <string>

#include "url/gurl.h"

namespace ads {

struct CatalogNotificationAdPayloadInfo final {
  CatalogNotificationAdPayloadInfo();
  CatalogNotificationAdPayloadInfo(
      const CatalogNotificationAdPayloadInfo& info);
  CatalogNotificationAdPayloadInfo& operator=(
      const CatalogNotificationAdPayloadInfo& info);
  ~CatalogNotificationAdPayloadInfo();

  bool operator==(const CatalogNotificationAdPayloadInfo& rhs) const;
  bool operator!=(const CatalogNotificationAdPayloadInfo& rhs) const;

  std::string body;
  std::string title;
  GURL target_url;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NOTIFICATION_AD_CATALOG_NOTIFICATION_AD_PAYLOAD_INFO_H_
