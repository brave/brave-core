/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_UTIL_H_

#include "absl/types/optional.h"

namespace ads {

namespace security {
struct VerifiableConversionEnvelopeInfo;
}  // namespace security

struct ConversionQueueItemInfo;

namespace user_data {

absl::optional<security::VerifiableConversionEnvelopeInfo> GetEnvelope(
    const ConversionQueueItemInfo& conversion_queue_item);

}  // namespace user_data
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_UTIL_H_
