/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_BASE_H_

#include "base/memory/raw_ptr.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info_aliases.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_aliases.h"

namespace ads {

namespace ad_targeting {
struct UserModelInfo;
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace new_tab_page_ads {

class EligibleAdsBase {
 public:
  EligibleAdsBase(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource);
  virtual ~EligibleAdsBase();

  virtual void GetForUserModel(
      const ad_targeting::UserModelInfo& user_model,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) = 0;

  void set_last_served_ad(const AdInfo& ad) { last_served_ad_ = ad; }

 protected:
  raw_ptr<ad_targeting::geographic::SubdivisionTargeting>
      subdivision_targeting_ = nullptr;  // NOT OWNED

  raw_ptr<resource::AntiTargeting> anti_targeting_resource_ =
      nullptr;  // NOT OWNED

  AdInfo last_served_ad_;
};

}  // namespace new_tab_page_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_BASE_H_
