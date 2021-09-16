/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_AD_NOTIFICATION_PAYLOAD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_AD_NOTIFICATION_PAYLOAD_INFO_H_

#include <string>

namespace ads {

struct CatalogAdNotificationPayloadInfo final {
  CatalogAdNotificationPayloadInfo();
  CatalogAdNotificationPayloadInfo(
      const CatalogAdNotificationPayloadInfo& info);
  ~CatalogAdNotificationPayloadInfo();

  bool operator==(const CatalogAdNotificationPayloadInfo& rhs) const;
  bool operator!=(const CatalogAdNotificationPayloadInfo& rhs) const;

  std::string body;
  std::string title;
  std::string target_url;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_AD_NOTIFICATION_PAYLOAD_INFO_H_
