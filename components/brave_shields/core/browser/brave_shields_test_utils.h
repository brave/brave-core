// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_TEST_UTILS_H_

#include <stdint.h>

#include <vector>

#include "base/auto_reset.h"
#include "base/containers/flat_set.h"
#include "base/token.h"

namespace brave_shields {

class ScopedStableFarblingTokensForTesting {
 public:
  // The seed value determines whether the farbling is random or deterministic.
  // If the seed is 0, the farbling is random (production mode). If the seed is
  // non-zero, the farbling is deterministic.
  explicit ScopedStableFarblingTokensForTesting(uint32_t seed);
  ~ScopedStableFarblingTokensForTesting();

 private:
  base::AutoReset<uint32_t> scoped_stable_farbling_token_seed_;
};

// RAII guard that sets a list of allowlisted profile tokens for the duration of
// a test and resets the global back to std::nullopt on destruction, preventing
// test-state leakage.
class ScopedAllowlistedProfileTokensForTesting {
 public:
  // An allowlist of profile tokens which is checked with
  // BraveShieldsSettingsService |profile_level_farbling_entropy_| to then
  // selectively allow adding noise from the token if it's present. This
  // is useful when the farbling tests are controlled, but we still need to add
  // profile level noise. See BraveShieldsSettingsService
  // set_profile_level_farbling_entropy_for_testing for more details on
  // controlled farbling tests.
  explicit ScopedAllowlistedProfileTokensForTesting(
      std::vector<base::Token> tokens);
  ~ScopedAllowlistedProfileTokensForTesting();

  ScopedAllowlistedProfileTokensForTesting(
      const ScopedAllowlistedProfileTokensForTesting&) = delete;
  ScopedAllowlistedProfileTokensForTesting& operator=(
      const ScopedAllowlistedProfileTokensForTesting&) = delete;

 private:
  base::AutoReset<std::optional<base::flat_set<base::Token>>> profile_tokens_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_TEST_UTILS_H_
