/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_BUILDER_H_

#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

struct ConversionInfo;

absl::optional<ConversionInfo> BuildConversion(
    const mojom::SearchResultAdInfoPtr& ad_mojom);

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_BUILDER_H_
