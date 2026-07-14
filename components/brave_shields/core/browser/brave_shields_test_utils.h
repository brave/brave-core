// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_TEST_UTILS_H_

#include <stdint.h>

#include "base/auto_reset.h"
#include "base/token.h"

namespace brave_shields {

class ScopedStableFarblingTokensForTesting {
 public:
  // The seed value determines whether the farbling is random or deterministic.
  // If the seed is 0, the farbling is random (production mode). If the seed is
  // non-zero, the farbling is deterministic.
  // |profile_token| set to an empty base::Token() removes the randomness
  // introduced by the additonal profile level entropy.
  explicit ScopedStableFarblingTokensForTesting(uint32_t seed,
                                                base::Token profile_token);
  ~ScopedStableFarblingTokensForTesting();

 private:
  base::AutoReset<uint32_t> scoped_stable_farbling_token_seed_;
  base::AutoReset<std::optional<base::Token>> profile_token_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_TEST_UTILS_H_
