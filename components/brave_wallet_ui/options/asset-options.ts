// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// magics
import { SKIP_PRICE_LOOKUP_COINGECKO_ID } from '../common/constants/magics'

// types
import { BraveWallet, SupportedTestNetworks } from '../constants/types'

// options
import { AllNetworksOption } from './network-filter-options'

// utils
import { isComponentInStorybook } from '../utils/string-utils'

// icons
import {
  ETHIconUrl,
  SOLIconUrl,
  AVAXIconUrl,
  BNBIconUrl,
  BTCIconUrl,
  ZECIconUrl,
  FILECOINIconUrl
} from '../stories/mock-data/asset-icons'

const isStorybook = isComponentInStorybook()

export const getNetworkLogo = (chainId: string, symbol: string): string => {
  if (chainId === BraveWallet.AURORA_MAINNET_CHAIN_ID)
    return 'chrome://erc-token-images/aurora.png'
  if (chainId === BraveWallet.OPTIMISM_MAINNET_CHAIN_ID)
    return 'chrome://erc-token-images/op.png'
  if (chainId === BraveWallet.POLYGON_MAINNET_CHAIN_ID)
    return 'chrome://erc-token-images/matic.png'
  if (chainId === BraveWallet.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID)
    return isStorybook ? BNBIconUrl : 'chrome://erc-token-images/bnb.png'
  if (chainId === BraveWallet.AVALANCHE_MAINNET_CHAIN_ID)
    return isStorybook ? AVAXIconUrl : 'chrome://erc-token-images/avax.png'
  if (chainId === BraveWallet.FANTOM_MAINNET_CHAIN_ID)
    return 'chrome://erc-token-images/ftm.png'
  if (chainId === BraveWallet.CELO_MAINNET_CHAIN_ID)
    return 'chrome://erc-token-images/celo.png'
  if (chainId === BraveWallet.ARBITRUM_MAINNET_CHAIN_ID)
    return 'chrome://erc-token-images/arb.png'
  if (chainId === BraveWallet.NEON_EVM_MAINNET_CHAIN_ID)
    return 'chrome://erc-token-images/neon.png'
  if (chainId === BraveWallet.BASE_MAINNET_CHAIN_ID)
    return 'chrome://erc-token-images/base.png'
  if (chainId === AllNetworksOption.chainId)
    return AllNetworksOption.iconUrls[0]

  switch (symbol.toUpperCase()) {
    case 'SOL':
      return isStorybook ? SOLIconUrl : 'chrome://erc-token-images/sol.png'
    case 'ETH':
      return isStorybook ? ETHIconUrl : 'chrome://erc-token-images/eth.png'
    case 'FIL':
      return isStorybook ? FILECOINIconUrl : 'chrome://erc-token-images/fil.png'
    case 'BTC':
      return isStorybook ? BTCIconUrl : 'chrome://erc-token-images/btc.png'
    case 'ZEC':
      return isStorybook ? ZECIconUrl : 'chrome://erc-token-images/zec.png'
  }

  return ''
}

export const makeNativeAssetLogo = (symbol: string, chainId: string) => {
  return getNetworkLogo(
    symbol.toUpperCase() === 'ETH' ? BraveWallet.MAINNET_CHAIN_ID : chainId,
    symbol
  )
}

type UndefinedIf<R, T> = T extends undefined ? undefined : R
export const makeNetworkAsset = <
  T extends BraveWallet.NetworkInfo | undefined | null
>(
  network: T
): UndefinedIf<BraveWallet.BlockchainToken, T> => {
  if (!network) {
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
    isSpam: false,
    decimals: network.decimals,
    visible: true,
    tokenId: '',
    coingeckoId:
      // skip getting prices of known testnet tokens
      // except Goerli ETH, which has real-world value
      SupportedTestNetworks.includes(network.chainId)
        ? network.chainId === BraveWallet.GOERLI_CHAIN_ID &&
          network.symbol.toLowerCase() === 'eth'
          ? 'goerli-eth'
          : SKIP_PRICE_LOOKUP_COINGECKO_ID
        : '',
    chainId: network.chainId,
    coin: network.coin
  } as UndefinedIf<BraveWallet.BlockchainToken, T>
}
