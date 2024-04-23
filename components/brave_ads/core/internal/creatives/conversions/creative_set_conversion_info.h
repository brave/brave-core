/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_INFO_H_

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"

namespace brave_ads {

struct CreativeSetConversionInfo final {
  CreativeSetConversionInfo();

  CreativeSetConversionInfo(const CreativeSetConversionInfo&);
  CreativeSetConversionInfo& operator=(const CreativeSetConversionInfo&);

  CreativeSetConversionInfo(CreativeSetConversionInfo&&) noexcept;
  CreativeSetConversionInfo& operator=(CreativeSetConversionInfo&&) noexcept;

  ~CreativeSetConversionInfo();

  bool operator==(const CreativeSetConversionInfo&) const = default;

  [[nodiscard]] bool IsValid() const;

  std::string id;
  std::string url_pattern;
  std::optional<std::string> verifiable_advertiser_public_key_base64;
  base::TimeDelta observation_window;
  std::optional<base::Time> expire_at;
};

using CreativeSetConversionList = std::vector<CreativeSetConversionInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_INFO_H_
