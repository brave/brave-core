// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet } from '../constants/types'

export const getPriceIdForToken = (
  token: Pick<
    BraveWallet.BlockchainToken,
    | 'contractAddress'
    | 'symbol'
    | 'coingeckoId'
    | 'chainId'
    >
) => {
  if (token?.coingeckoId) {
    return token.coingeckoId.toLowerCase()
  }

  const isEthereumNetwork = token.chainId === BraveWallet.MAINNET_CHAIN_ID
  if (isEthereumNetwork && token.contractAddress) {
    return token.contractAddress.toLowerCase()
  }

  return token.symbol.toLowerCase()
}
