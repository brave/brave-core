/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WALLET_CONNECT_WALLET_CONNECT_UTILS_H_
#define BRAVE_COMPONENTS_WALLET_CONNECT_WALLET_CONNECT_UTILS_H_

#include <string>

#include "brave/components/wallet_connect/wallet_connect.mojom.h"

namespace wallet_connect {

// WalletConnect URI Format specified in EIP-1328
mojom::WalletConnectURIDataPtr ParseWalletConnectURI(const std::string& uri);

}  // namespace wallet_connect

#endif  // BRAVE_COMPONENTS_WALLET_CONNECT_WALLET_CONNECT_UTILS_H_
