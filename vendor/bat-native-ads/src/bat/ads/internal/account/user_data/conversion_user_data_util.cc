/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/conversion_user_data_util.h"

#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/conversions/verifiable_conversion_info.h"
#include "bat/ads/internal/security/conversions/conversions_util.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace user_data {

absl::optional<security::VerifiableConversionEnvelopeInfo> GetEnvelope(
    const ConversionQueueItemInfo& conversion_queue_item) {
  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = conversion_queue_item.conversion_id;
  verifiable_conversion.public_key =
      conversion_queue_item.advertiser_public_key;

  if (!verifiable_conversion.IsValid()) {
    return absl::nullopt;
  }

  return security::SealEnvelope(verifiable_conversion);
}

}  // namespace user_data
}  // namespace ads
