/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_unittest_util.h"

#include "base/strings/string_util.h"

namespace brave_ads::test {

std::string BuildSubdivision(const std::string& country_code,
                             const std::string& subdivision_code) {
  return base::ReplaceStringPlaceholders(
      "$1-$2", {country_code, subdivision_code}, nullptr);
}

}  // namespace brave_ads::test
