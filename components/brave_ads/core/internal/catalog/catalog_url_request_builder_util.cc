/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_ads {

std::string BuildCatalogUrlPath() {
  return absl::StrFormat("/v%d/catalog", kCatalogVersion);
}

}  // namespace brave_ads
