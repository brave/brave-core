/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_BRAVE_WALLET_RESPONSE_HELPERS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_BRAVE_WALLET_RESPONSE_HELPERS_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "brave/components/brave_wallet/renderer/web3_provider_constants.h"

namespace brave_wallet {

std::unique_ptr<base::Value> FormProviderResponse(ProviderErrors code,
                                                  const std::string& message);
std::unique_ptr<base::Value> FormProviderResponse(
    const std::string& controller_response,
    bool* reject);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_BRAVE_WALLET_RESPONSE_HELPERS_H_
