/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_UNITS_PROMOTED_CONTENT_AD_PROMOTED_CONTENT_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_UNITS_PROMOTED_CONTENT_AD_PROMOTED_CONTENT_AD_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/public/export.h"
#include "brave/components/brave_ads/core/public/units/ad_info.h"

namespace brave_ads {

struct ADS_EXPORT PromotedContentAdInfo final : AdInfo {
  [[nodiscard]] bool IsValid() const;

  std::string title;
  std::string description;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_UNITS_PROMOTED_CONTENT_AD_PROMOTED_CONTENT_AD_INFO_H_
