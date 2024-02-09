/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_F4_JUMBLE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_F4_JUMBLE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"

// Implementation of F4Jumble https://zips.z.cash/zip-0316#solution
namespace brave_wallet {

std::optional<std::vector<uint8_t>> ApplyF4Jumble(
    base::span<const uint8_t> span);
std::optional<std::vector<uint8_t>> RevertF4Jumble(
    base::span<const uint8_t> jumbled_message);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_F4_JUMBLE_H_
