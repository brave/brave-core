/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_RANDOM_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_RANDOM_UTIL_H_

#include <stdint.h>

#include <string>

namespace brave_rewards::core {
namespace util {

// Generates a random 32-byte hex string.
std::string GenerateRandomHexString();

// Generates a PKCE-compatible code verifier.
std::string GeneratePKCECodeVerifier();

// Generates a PKCE-compatible code challenge based on |code_verifier|.
std::string GeneratePKCECodeChallenge(const std::string& code_verifier);

}  // namespace util
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_RANDOM_UTIL_H_
