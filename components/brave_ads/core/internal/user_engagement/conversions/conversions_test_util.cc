/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_util.h"

#include "url/gurl.h"

namespace brave_ads::test {

// See `conversions_test_constants.h` for URL patterns that match and do not
// match.

std::vector<GURL> BuildDefaultConversionRedirectChain() {
  // Matches `kMatchingUrlPattern` and `kAnotherMatchingUrlPattern` in
  // `conversions_test_constants.h`.
  return {GURL("https://foo.com/bar"), GURL("https://qux.com/quux/corge")};
}

}  // namespace brave_ads::test
