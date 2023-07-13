/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_builder_util.h"

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"

namespace brave_ads {

bool ShouldExtractVerifiableConversionId(
    const CreativeSetConversionInfo& creative_set_conversion) {
  return creative_set_conversion.extract_verifiable_id &&
         !creative_set_conversion.verifiable_advertiser_public_key_base64
              .empty();
}

}  // namespace brave_ads
