/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_builder_util.h"

#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_ads {

std::string BuildSubdivisionUrlPath() {
  return absl::StrFormat("/v%d/getstate", kSubdivisionServerVersion);
}

}  // namespace brave_ads
