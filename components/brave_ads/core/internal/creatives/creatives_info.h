/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVES_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVES_INFO_H_

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"

namespace brave_ads {

struct CreativesInfo final {
  CreativesInfo();

  CreativesInfo(const CreativesInfo&);
  CreativesInfo& operator=(const CreativesInfo&);

  CreativesInfo(CreativesInfo&&) noexcept;
  CreativesInfo& operator=(CreativesInfo&&) noexcept;

  ~CreativesInfo();

  CreativeInlineContentAdList inline_content_ads;
  CreativeNewTabPageAdList new_tab_page_ads;
  CreativeNotificationAdList notification_ads;
  CreativePromotedContentAdList promoted_content_ads;

  CreativeSetConversionList conversions;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVES_INFO_H_
