/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MOJOM_UINT128_MOJOM_TRAITS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MOJOM_UINT128_MOJOM_TRAITS_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"  // IWYU pragma: export
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace mojo {

template <>
struct StructTraits<::brave_wallet::mojom::uint128DataView,
                    ::brave_wallet::uint128_t> {
  static std::uint64_t high(const ::brave_wallet::uint128_t& input) {
    return input >> 64;
  }

  static std::uint64_t low(const ::brave_wallet::uint128_t& input) {
    return input & 0xffffffffffffffff;
  }

  static bool Read(brave_wallet::mojom::uint128DataView data,
                   ::brave_wallet::uint128_t* out) {
    brave_wallet::uint128_t x = (brave_wallet::uint128_t{data.high()} << 64) |
                                brave_wallet::uint128_t{data.low()};
    *out = x;
    return true;
  }
};

}  // namespace mojo

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MOJOM_UINT128_MOJOM_TRAITS_H_
