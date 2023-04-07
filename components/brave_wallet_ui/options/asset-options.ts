// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'
import { AllNetworksOption } from './network-filter-options'

export const getNetworkLogo = (chainId: string, symbol: string): string => {
  switch (true) {
    case chainId === BraveWallet.AURORA_MAINNET_CHAIN_ID:
      return 'chrome://erc-token-images/aurora.png'
    case chainId === BraveWallet.OPTIMISM_MAINNET_CHAIN_ID:
      return 'chrome://erc-token-images/op.png'
    case chainId === BraveWallet.POLYGON_MAINNET_CHAIN_ID:
      return 'chrome://erc-token-images/matic.png'
    case chainId === BraveWallet.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID:
      return 'chrome://erc-token-images/bnb.png'
    case chainId === BraveWallet.AVALANCHE_MAINNET_CHAIN_ID:
      return 'chrome://erc-token-images/avax.png'
    case chainId === BraveWallet.FANTOM_MAINNET_CHAIN_ID:
      return 'chrome://erc-token-images/ftm.png'
    case chainId === BraveWallet.CELO_MAINNET_CHAIN_ID:
      return 'chrome://erc-token-images/celo.png'
    case chainId === BraveWallet.ARBITRUM_MAINNET_CHAIN_ID:
      return 'chrome://erc-token-images/arb.png'
    case chainId === AllNetworksOption.chainId:
      return AllNetworksOption.iconUrls[0]
    case symbol.toUpperCase() === 'SOL':
      return 'chrome://erc-token-images/sol.png'
    case symbol.toUpperCase() === 'FIL':
      return 'chrome://erc-token-images/fil.png'
    case symbol.toUpperCase() === 'ETH':
      return 'chrome://erc-token-images/eth.png'
    default: return ''
  }
}

export const makeNativeAssetLogo = (symbol: string, chainId: string) => {
  return getNetworkLogo(
    symbol.toUpperCase() === 'ETH'
      ? BraveWallet.MAINNET_CHAIN_ID
      : chainId,
    symbol
  )
}

type UndefinedIf<R, T> = T extends undefined ? undefined : R
export const makeNetworkAsset = <T extends BraveWallet.NetworkInfo | undefined>(
  network: T
): UndefinedIf<BraveWallet.BlockchainToken, T> => {
  if (network === undefined) {
    return undefined as UndefinedIf<BraveWallet.BlockchainToken, T>
  }

  return {
    contractAddress: '',
    name: network.symbolName,
    symbol: network.symbol,
    logo: makeNativeAssetLogo(network.symbol, network.chainId),
    isErc20: false,
    isErc721: false,
    isErc1155: false,
    isNft: false,
    decimals: network.decimals,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    chainId: network.chainId,
    coin: network.coin
  } as UndefinedIf<BraveWallet.BlockchainToken, T>
}
