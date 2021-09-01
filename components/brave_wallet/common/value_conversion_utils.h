/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_VALUE_CONVERSION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_VALUE_CONVERSION_UTILS_H_

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

base::Value EthereumChainToValue(const mojom::EthereumChainPtr& chain);
absl::optional<mojom::EthereumChain> ValueToEthereumChain(
    const base::Value& value);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_VALUE_CONVERSION_UTILS_H_
