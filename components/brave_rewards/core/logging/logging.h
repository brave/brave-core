/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_LOGGING_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_LOGGING_H_

#include <ostream>
#include <string>

#include "base/logging.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "brave/components/brave_rewards/core/logging/logging_util.h"

namespace brave_rewards::internal {

void set_client_for_logging(mojom::RewardsEngineClient* client);

void Log(const char* file,
         const int line,
         const int verbose_level,
         const std::string& message);

// |verbose_level| is an arbitrary integer value (higher numbers should be used
// for more verbose logging), so you can make your logging levels as granular as
// you wish and can be adjusted on a per-module basis at runtime. Default is 0
//
// Example usage:
//
//   --enable-logging=stderr --v=1 --vmodule=foo=2,bar=3
//
// This runs BAT Rewards with the global VLOG level set to "print everything at
// level 1 and lower", but prints levels up to 2 in foo.cc and levels up to 3 in
// bar.cc
//
// Any pattern containing a forward or backward slash will be tested against the
// whole pathname and not just the module. e.g., "/foo/bar/=2" would change the
// logging level for all code in source files under a "foo/bar" directory
//
// BAT Rewards verbose levels:
//
//   0 Error
//   1 Info
//   5 URL request
//   6 URL response
//   7 URL response (with large body)
//   8 Database queries
//   9 Detailed debugging (response headers, etc)

#define BLOG(verbose_level, stream)                               \
  brave_rewards::internal::Log(__FILE__, __LINE__, verbose_level, \
                               (std::ostringstream() << stream).str());

// You can also do conditional verbose logging when some extra computation and
// preparation for logs is not needed:
//
//   BLOG_IF(2, bat_tokens < 10, "Got too few Basic Attention Tokens!");

#define BLOG_IF(verbose_level, condition, stream) \
  !(condition) ? (void)0 : BLOG(verbose_level, stream)

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_LOGGING_H_
