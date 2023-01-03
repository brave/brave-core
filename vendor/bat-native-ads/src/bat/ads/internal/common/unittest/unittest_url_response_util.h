/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_URL_RESPONSE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_URL_RESPONSE_UTIL_H_

#include "absl/types/optional.h"
#include "bat/ads/internal/common/unittest/unittest_url_response_alias.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads {

absl::optional<mojom::UrlResponseInfo> GetNextUrlResponseForRequest(
    const mojom::UrlRequestInfoPtr& url_request,
    const URLResponseMap& url_responses);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_URL_RESPONSE_UTIL_H_
