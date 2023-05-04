/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_RESPONSE_HELPERS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_RESPONSE_HELPERS_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

base::Value GetProviderRequestReturnFromEthJsonResponse(
    int http_code,
    const base::Value& json_value,
    bool* reject);
base::Value GetProviderErrorDictionary(mojom::ProviderError code,
                                       const std::string& message);
base::Value GetSolanaProviderErrorDictionary(mojom::SolanaProviderError code,
                                             const std::string& message);
base::Value ToProviderResponse(base::Value id,
                               base::Value* result,
                               base::Value* error);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_RESPONSE_HELPERS_H_
