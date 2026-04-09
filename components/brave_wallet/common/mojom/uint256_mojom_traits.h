/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MOJOM_UINT256_MOJOM_TRAITS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MOJOM_UINT256_MOJOM_TRAITS_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"  // IWYU pragma: export

namespace brave_wallet {
using uint256_t = unsigned _BitInt(256);
}

namespace mojo {
template <>
struct StructTraits<::brave_wallet::mojom::Uint256DataView,
                    ::brave_wallet::uint256_t> {
  static std::string uint256_hex(const ::brave_wallet::uint256_t& input);

  static bool Read(brave_wallet::mojom::Uint256DataView data,
                   ::brave_wallet::uint256_t* out);
};

}  // namespace mojo

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MOJOM_UINT256_MOJOM_TRAITS_H_
