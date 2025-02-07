/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_COMMON_SKUS_UTILS_H_
#define BRAVE_COMPONENTS_SKUS_COMMON_SKUS_UTILS_H_

#include <array>
#include <string>

class GURL;

namespace skus {
// NOTE: please open a security review when appending to this list.
constexpr auto kSafeOrigins = std::to_array<std::string_view>({
    "https://account.brave.com",
    "https://account.bravesoftware.com",
    "https://account.brave.software",
});

// This version is safe for use elsewhere. The internal `IsSameOriginWith`
// check is different than the version above.
//
// See //url/origin.cc
bool IsSafeOrigin(const GURL& origin);
}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_COMMON_SKUS_UTILS_H_
