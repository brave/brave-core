/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_LOGGING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_LOGGING_UTIL_H_

#include <sstream>  // IWYU pragma: keep

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"  // IWYU pragma: keep

namespace brave_ads {

// `verbose_level` is an arbitrary integer value (higher numbers should be used
// for more verbose logging), so you can make your logging levels as granular as
// you wish and can be adjusted on a per-module basis at runtime. Defaults to 0
//
// Example usage:
//
//   --enable-logging=stderr --v=1 --vmodule=foo=2,bar=3
//
// This runs Brave Ads with the global VLOG level set to "print everything at
// level 1 and lower", but prints levels up to 2 in foo.cc and levels up to 3 in
// bar.cc
//
// Any pattern containing a forward or backward slash will be tested against the
// whole pathname and not just the module. e.g., "/foo/bar/=2" would change the
// logging level for all code in source files under a "foo/bar" directory
//
// Brave Ads verbosity levels:
//
//   0 Error
//   1 Info
//   5 URL request
//   6 URL response
//   7 URL response (with large body), response headers and request headers
//   8 Database queries

#define BLOG(verbose_level, stream)      \
  Log(__FILE__, __LINE__, verbose_level, \
      (std::ostringstream() << stream).str());  // NOLINT

// You can also do conditional verbose logging when some extra computation and
// preparation for logs is not needed:
//
//   BLOG_IF(2, bat_tokens < 10, "Got too few Basic Attention Tokens!");

#define BLOG_IF(verbose_level, condition, stream) \
  !(condition) ? (void)0 : BLOG(verbose_level, stream)

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_LOGGING_UTIL_H_
