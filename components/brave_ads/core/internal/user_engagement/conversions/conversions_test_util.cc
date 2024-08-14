/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_util.h"

#include "url/gurl.h"

namespace brave_ads::test {

// See `test/data/resources/nnqccijfhvzwyrxpxwjrpmynaiazctqb` for conversion
// resource test data.

// Also, see `conversions_test_constants.h` for URL patterns that match and do
// not match, as well as patterns for verifiable conversion ids.

std::vector<GURL> BuildDefaultConversionRedirectChain() {
  // Matches `kMatchingUrlPattern` and `kAnotherMatchingUrlPattern` in
  // `conversions_test_constants.h`.
  return {GURL("https://foo.com/bar"), GURL("https://qux.com/quux/corge")};
}

std::vector<GURL> BuildVerifiableConversionRedirectChain() {
  // Matches `kMatchingVerifiableConversionUrlPattern` in
  // `conversions_test_constants.h`.
  return {GURL("https://foo.com/bar?qux_id=xyzzy")};
}

std::string BuildVerifiableConversionUrlPattern() {
  // Matches redirect chain in `BuildVerifiableConversionRedirectChain`.
  return "https://foo.com/bar?qux_id=*";
}

}  // namespace brave_ads::test
