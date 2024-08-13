// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export type ChainBalances = {
  tokenBalances: Record<string, string> // key = assetId
}

export type AccountBalances = {
  chains: Record<string, ChainBalances> // key = chainId
}

export type TokenBalancesRegistry = {
  accounts: Record<string, AccountBalances> // key = uniqueKey
}
