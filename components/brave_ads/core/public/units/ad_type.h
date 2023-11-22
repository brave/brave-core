/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_UNITS_AD_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_UNITS_AD_TYPE_H_

#include <ostream>
#include <string_view>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

// An enum with the codified ad types.
enum class AdType {
  kUndefined,
  kNotificationAd,
  kNewTabPageAd,
  kPromotedContentAd,
  kInlineContentAd,
  kSearchResultAd
};

// Returns a string constant for a given AdType value
const char* ToString(AdType type);

// Creates an AdType instance with a value corresponding the string provided.
AdType ParseAdType(std::string_view value);

// Creates an AdType instance from a mojom::AdType instance.
AdType FromMojomTypeToAdType(mojom::AdType value);

std::ostream& operator<<(std::ostream& os, AdType type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_UNITS_AD_TYPE_H_
