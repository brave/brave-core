/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_PERMISSION_RULES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_PERMISSION_RULES_H_

#include "bat/ads/internal/ad_events/ad_event_info.h"

namespace ads {

struct AdInfo;

namespace promoted_content_ads {
namespace frequency_capping {

class PermissionRules {
 public:
  PermissionRules();
  ~PermissionRules();

  bool HasPermission() const;

 private:
  PermissionRules(const PermissionRules&) = delete;
  PermissionRules& operator=(const PermissionRules&) = delete;
};

}  // namespace frequency_capping
}  // namespace promoted_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_PERMISSION_RULES_H_
