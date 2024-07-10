/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_BUILDER_H_

#include <string>

namespace brave_ads {

struct CreativeNewTabPageAdInfo;
struct NewTabPageAdInfo;

NewTabPageAdInfo BuildNewTabPageAd(const CreativeNewTabPageAdInfo& creative_ad);

NewTabPageAdInfo BuildNewTabPageAd(const std::string& placement_id,
                                   const CreativeNewTabPageAdInfo& creative_ad);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_BUILDER_H_
