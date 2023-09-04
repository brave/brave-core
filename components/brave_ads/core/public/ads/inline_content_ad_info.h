/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_INLINE_CONTENT_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_INLINE_CONTENT_AD_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/public/ad_info.h"
#include "brave/components/brave_ads/core/public/export.h"
#include "url/gurl.h"

namespace brave_ads {

struct ADS_EXPORT InlineContentAdInfo final : AdInfo {
  InlineContentAdInfo();

  InlineContentAdInfo(const InlineContentAdInfo&);
  InlineContentAdInfo& operator=(const InlineContentAdInfo&);

  InlineContentAdInfo(InlineContentAdInfo&&) noexcept;
  InlineContentAdInfo& operator=(InlineContentAdInfo&&) noexcept;

  ~InlineContentAdInfo();

  bool operator==(const InlineContentAdInfo&) const;
  bool operator!=(const InlineContentAdInfo&) const;

  [[nodiscard]] bool IsValid() const;

  std::string title;
  std::string description;
  GURL image_url;
  std::string dimensions;
  std::string cta_text;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_INLINE_CONTENT_AD_INFO_H_
