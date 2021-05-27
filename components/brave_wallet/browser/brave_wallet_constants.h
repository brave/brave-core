/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONSTANTS_H_

namespace brave_wallet {

enum class Web3ProviderTypes {
  ASK,
  NONE,
  CRYPTO_WALLETS,
  METAMASK,
  BRAVE_WALLET
};

enum class Network {
  kMainnet,
  kRinkeby,
  kRopsten,
  kGoerli,
  kKovan,
  kLocalhost,
  kCustom,
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONSTANTS_H_
