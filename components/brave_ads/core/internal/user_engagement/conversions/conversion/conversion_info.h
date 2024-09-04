/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSION_CONVERSION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSION_CONVERSION_INFO_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

struct ConversionInfo final {
  ConversionInfo();

  ConversionInfo(const ConversionInfo&);
  ConversionInfo& operator=(const ConversionInfo&);

  ConversionInfo(ConversionInfo&&) noexcept;
  ConversionInfo& operator=(ConversionInfo&&) noexcept;

  ~ConversionInfo();

  bool operator==(const ConversionInfo&) const = default;

  [[nodiscard]] bool IsValid() const;

  mojom::AdType ad_type = mojom::AdType::kUndefined;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  std::string segment;
  ConversionActionType action_type = ConversionActionType::kUndefined;
  std::optional<VerifiableConversionInfo> verifiable;
};

using ConversionList = std::vector<ConversionInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSION_CONVERSION_INFO_H_
