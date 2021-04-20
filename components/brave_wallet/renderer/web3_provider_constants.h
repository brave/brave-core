/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_WEB3_PROVIDER_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_WEB3_PROVIDER_CONSTANTS_H_

namespace brave_wallet {

enum class ProviderErrors {
  kUserRejectedRequest = 4001,
  kUnauthorized = 4100,
  kUnsupportedMethod = 4200,
  kDisconnected = 4900,
  kChainDisconnected = 4901,
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_WEB3_PROVIDER_CONSTANTS_H_
