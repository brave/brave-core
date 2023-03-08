/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PROMOTED_CONTENT_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PROMOTED_CONTENT_AD_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/export.h"

namespace ads {

struct ADS_EXPORT PromotedContentAdInfo final : AdInfo {
  bool IsValid() const;

  std::string title;
  std::string description;
};

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PROMOTED_CONTENT_AD_INFO_H_
