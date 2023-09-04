/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_CONSTANTS_H_

#include <string>
#include <tuple>

#include "base/functional/callback.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace wireguard {

using BooleanCallback = base::OnceCallback<void(bool)>;
using WireguardKeyPair = absl::optional<std::tuple<std::string, std::string>>;
using WireguardGenerateKeypairCallback =
    base::OnceCallback<void(WireguardKeyPair)>;

}  // namespace wireguard

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIREGUARD_CONSTANTS_H_
