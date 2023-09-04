/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_CONVERSION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_CONVERSION_INFO_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/public/ad_type.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct ConversionInfo final {
  ConversionInfo();

  ConversionInfo(const ConversionInfo&);
  ConversionInfo& operator=(const ConversionInfo&);

  ConversionInfo(ConversionInfo&&) noexcept;
  ConversionInfo& operator=(ConversionInfo&&) noexcept;

  ~ConversionInfo();

  [[nodiscard]] bool IsValid() const;

  AdType ad_type = AdType::kUndefined;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  std::string segment;
  ConversionActionType action_type = ConversionActionType::kUndefined;
  absl::optional<VerifiableConversionInfo> verifiable;
};

bool operator==(const ConversionInfo&, const ConversionInfo&);
bool operator!=(const ConversionInfo&, const ConversionInfo&);

using ConversionList = std::vector<ConversionInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_CONVERSION_INFO_H_
