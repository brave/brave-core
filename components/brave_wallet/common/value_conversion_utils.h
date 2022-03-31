/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_VALUE_CONVERSION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_VALUE_CONVERSION_UTILS_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

base::Value EthNetworkInfoToValue(const mojom::NetworkInfoPtr& info);
mojom::NetworkInfoPtr ValueToEthNetworkInfo(const base::Value& value);
base::ListValue PermissionRequestResponseToValue(
    const std::string& origin,
    const std::vector<std::string> accounts);

mojom::BlockchainTokenPtr ValueToBlockchainToken(const base::Value& value);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_VALUE_CONVERSION_UTILS_H_
