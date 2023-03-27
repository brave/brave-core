/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PREFIX_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PREFIX_UTIL_H_

#include <string>

namespace brave_rewards::core {
namespace publisher {

extern const size_t kMinPrefixSize;
extern const size_t kMaxPrefixSize;

// Returns a hash prefix for the specified publisher key
std::string GetHashPrefixRaw(const std::string& publisher_key,
                             size_t prefix_size);

// Retuns a hash prefix as a hex string for the specified publisher key
std::string GetHashPrefixInHex(const std::string& publisher_key,
                               size_t prefix_size);

}  // namespace publisher
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PREFIX_UTIL_H_
