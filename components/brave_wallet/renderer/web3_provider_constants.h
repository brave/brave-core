/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_WEB3_PROVIDER_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_WEB3_PROVIDER_CONSTANTS_H_

namespace brave_wallet {

enum class ProviderErrors {
  kUserRejectedRequest = 4001,  // User rejected the request
  kUnauthorized = 4100,         // The requested account and/or method has not
                                // been authorized by the user
  kUnsupportedMethod = 4200,    // The requested method is not supported by this
                                // Ethereum provider
  kDisconnected = 4900,         // The provider is disconnected from all chains
  kChainDisconnected = 4901,    // The provider is disconnected from the
                                // specified chain
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_WEB3_PROVIDER_CONSTANTS_H_
