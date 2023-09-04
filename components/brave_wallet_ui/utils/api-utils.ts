// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { SKIP_PRICE_LOOKUP_COINGECKO_ID } from '../common/constants/magics'
import { BraveWallet, SupportedTestNetworks } from '../constants/types'

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

  // Skip price of testnet tokens other than goerli-eth
  if (SupportedTestNetworks.includes(token.chainId)) {
    // Goerli ETH has a real-world value
    if (
      token.chainId === BraveWallet.GOERLI_CHAIN_ID &&
      !token.contractAddress
    ) {
      return 'goerli-eth' // coingecko id
    }
    return SKIP_PRICE_LOOKUP_COINGECKO_ID
  }

  const isEthereumNetwork = token.chainId === BraveWallet.MAINNET_CHAIN_ID
  if (isEthereumNetwork && token.contractAddress) {
    return token.contractAddress.toLowerCase()
  }

  return token.symbol.toLowerCase()
}

export function handleEndpointError(
  endpointName: string,
  friendlyMessage: string,
  error: any
) {
  const message = `${friendlyMessage}: ${error?.message || error}`
  console.log(`error in: ${endpointName || 'endpoint'}: ${message}`)
  console.error(error)
  return {
    error: friendlyMessage
  }
}
