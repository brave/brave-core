/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/locale/subdivision_unittest_util.h"

#include "base/strings/string_util.h"

namespace brave_ads {

std::string BuildSubdivisionForTesting(const std::string& country,
                                       const std::string& region) {
  return base::ReplaceStringPlaceholders("$1-$2", {country, region}, nullptr);
}

}  // namespace brave_ads
