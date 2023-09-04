/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct CreativeSetConversionInfo final {
  CreativeSetConversionInfo();

  CreativeSetConversionInfo(const CreativeSetConversionInfo&);
  CreativeSetConversionInfo& operator=(const CreativeSetConversionInfo&);

  CreativeSetConversionInfo(CreativeSetConversionInfo&&) noexcept;
  CreativeSetConversionInfo& operator=(CreativeSetConversionInfo&&) noexcept;

  ~CreativeSetConversionInfo();

  [[nodiscard]] bool IsValid() const;

  std::string id;
  std::string url_pattern;
  absl::optional<std::string> verifiable_advertiser_public_key_base64;
  base::TimeDelta observation_window;
  base::Time expire_at;
};

bool operator==(const CreativeSetConversionInfo&,
                const CreativeSetConversionInfo&);
bool operator!=(const CreativeSetConversionInfo&,
                const CreativeSetConversionInfo&);

using CreativeSetConversionList = std::vector<CreativeSetConversionInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_INFO_H_
