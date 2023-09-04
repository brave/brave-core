/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/public/ad_type.h"
#include "brave/components/brave_ads/core/public/export.h"
#include "url/gurl.h"

namespace brave_ads {

struct ADS_EXPORT AdInfo {
  AdInfo();

  AdInfo(const AdInfo&);
  AdInfo& operator=(const AdInfo&);

  AdInfo(AdInfo&&) noexcept;
  AdInfo& operator=(AdInfo&&) noexcept;

  ~AdInfo();

  bool operator==(const AdInfo&) const;
  bool operator!=(const AdInfo&) const;

  [[nodiscard]] bool IsValid() const;

  AdType type = AdType::kUndefined;
  std::string placement_id;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  std::string segment;
  GURL target_url;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_INFO_H_
