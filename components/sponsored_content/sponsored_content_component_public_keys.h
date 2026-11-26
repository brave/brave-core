/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COMPONENT_PUBLIC_KEYS_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COMPONENT_PUBLIC_KEYS_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "crypto/hash.h"

namespace sponsored_content {

using ComponentPublicKeySHA256 = std::array<uint8_t, crypto::hash::kSha256Size>;

// Returns the sponsored content component's public key SHA256 hash for the
// given country code, or `std::nullopt` if none is available.
std::optional<ComponentPublicKeySHA256> GetComponentPublicKeySHA256(
    std::string_view country_code);

}  // namespace sponsored_content

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COMPONENT_PUBLIC_KEYS_H_
