/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_builder.h"

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/verifiable_conversion_id_pattern_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "url/gurl.h"

namespace brave_ads {

std::optional<VerifiableConversionInfo> MaybeBuildVerifiableConversion(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const ConversionResourceIdPatternMap& resource_id_patterns,
    const CreativeSetConversionInfo& creative_set_conversion) {
  if (!creative_set_conversion.verifiable_advertiser_public_key_base64) {
    return std::nullopt;
  }

  const std::optional<std::string> verifiable_conversion_id =
      MaybeParseVerifiableConversionId(redirect_chain, html,
                                       resource_id_patterns);
  if (!verifiable_conversion_id) {
    return std::nullopt;
  }

  return VerifiableConversionInfo{
      *verifiable_conversion_id,
      *creative_set_conversion.verifiable_advertiser_public_key_base64};
}

}  // namespace brave_ads
