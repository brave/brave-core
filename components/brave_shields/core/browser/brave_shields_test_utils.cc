// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_test_utils.h"

#include "base/token.h"

namespace brave_shields {

// Declared in brave_shields_settings_service.cc.
extern uint32_t g_stable_farbling_tokens_seed;
extern std::optional<base::Token> g_profile_token_allowed_for_testing;

ScopedStableFarblingTokensForTesting::ScopedStableFarblingTokensForTesting(
    uint32_t seed)
    : scoped_stable_farbling_token_seed_(&g_stable_farbling_tokens_seed, seed) {
}

ScopedStableFarblingTokensForTesting::~ScopedStableFarblingTokensForTesting() =
    default;

ScopedProfileTokenForTesting::ScopedProfileTokenForTesting(base::Token token)
    : profile_token_(&g_profile_token_allowed_for_testing, token) {}

ScopedProfileTokenForTesting::~ScopedProfileTokenForTesting() = default;

}  // namespace brave_shields
