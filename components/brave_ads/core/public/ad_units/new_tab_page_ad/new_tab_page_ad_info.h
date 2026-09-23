/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_INFO_H_

#include <string>

#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace brave_ads {

struct NewTabPageAdInfo final : AdInfo {
  [[nodiscard]] bool IsValid() const;

  std::string company_name;
  std::string alt;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_INFO_H_
