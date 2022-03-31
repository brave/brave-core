/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_FACTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_FACTORY_H_

#include <memory>

namespace ads {

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace new_tab_page_ads {

class EligibleAdsBase;

class EligibleAdsFactory final {
 public:
  static std::unique_ptr<EligibleAdsBase> Build(
      const int version,
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource);
};

}  // namespace new_tab_page_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_FACTORY_H_
