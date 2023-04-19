/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_FACTORY_H_

#include <memory>

namespace brave_ads {

namespace resource {
class AntiTargeting;
}  // namespace resource

class SubdivisionTargeting;

namespace new_tab_page_ads {

class EligibleAdsBase;

class EligibleAdsFactory final {
 public:
  static std::unique_ptr<EligibleAdsBase> Build(
      int version,
      const SubdivisionTargeting& subdivision_targeting,
      const resource::AntiTargeting& anti_targeting_resource);
};

}  // namespace new_tab_page_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_FACTORY_H_
