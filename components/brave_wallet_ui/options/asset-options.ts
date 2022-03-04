// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,I
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'
import {
  ALGOIconUrl,
  BATIconUrl,
  BNBIconUrl,
  BTCIconUrl,
  ETHIconUrl,
  ZRXIconUrl
} from '../assets/asset-icons'
import {
  CeloIcon,
  FantomIcon,
  OptimismIcon
} from '../assets/network-icons'
import MoonCatIcon from '../assets/png-icons/mooncat.png'

export function makeNetworkAsset (network: BraveWallet.NetworkInfo) {
  let logo
  switch (true) {
    case network.chainId === BraveWallet.OPTIMISM_MAINNET_CHAIN_ID:
      logo = OptimismIcon
      break

    case network.chainId === BraveWallet.POLYGON_MAINNET_CHAIN_ID:
      logo = 'chrome://erc-token-images/matic.png'
      break

    case network.chainId === BraveWallet.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID:
      logo = BNBIconUrl
      break

    case network.chainId === BraveWallet.AVALANCHE_MAINNET_CHAIN_ID:
      logo = 'chrome://erc-token-images/avax.png'
      break

    case network.chainId === BraveWallet.FANTOM_MAINNET_CHAIN_ID:
      logo = FantomIcon
      break

    case network.chainId === BraveWallet.CELO_MAINNET_CHAIN_ID:
      logo = CeloIcon
      break

    case network.symbol.toUpperCase() === 'ETH':
      logo = 'chrome://erc-token-images/eth.png'
      break

    default:
      logo = ''
  }

  return {
    contractAddress: '0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee',
    name: network.symbolName,
    symbol: network.symbol,
    logo: logo,
    isErc20: false,
    isErc721: false,
    decimals: network.decimals,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  } as BraveWallet.BlockchainToken
}

export const ETH = {
  contractAddress: '0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee',
  name: 'Ethereum',
  symbol: 'ETH',
  logo: ETHIconUrl,
  isErc20: false,
  isErc721: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: ''
} as BraveWallet.BlockchainToken

export const BAT = {
  contractAddress: '0x0D8775F648430679A709E98d2b0Cb6250d2887EF',
  name: 'Basic Attention Token',
  symbol: 'BAT',
  logo: 'chrome://erc-token-images/bat.png',
  isErc20: true,
  isErc721: false,
  decimals: 18,
  visible: false,
  tokenId: '',
  coingeckoId: ''
} as BraveWallet.BlockchainToken

// Use only with storybook as dummy data.
export const NewAssetOptions: BraveWallet.BlockchainToken[] = [
  {
    contractAddress: '1',
    name: 'Ethereum',
    symbol: 'ETH',
    logo: ETHIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '2',
    name: 'Basic Attention Token',
    symbol: 'BAT',
    logo: BATIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '3',
    name: 'Binance Coin',
    symbol: 'BNB',
    logo: BNBIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '4',
    name: 'Bitcoin',
    symbol: 'BTC',
    logo: BTCIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '5',
    name: 'Algorand',
    symbol: 'ALGO',
    logo: ALGOIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '6',
    name: '0x',
    symbol: 'ZRX',
    logo: ZRXIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '7',
    name: 'AcclimatedMoonCats',
    symbol: 'AMC',
    logo: MoonCatIcon,
    isErc20: false,
    isErc721: true,
    decimals: 0,
    visible: true,
    tokenId: '0x42a5',
    coingeckoId: ''
  }
]

// Use only with storybook as dummy data.
export const AccountAssetOptions: BraveWallet.BlockchainToken[] = [
  ETH,
  {
    contractAddress: '0x0D8775F648430679A709E98d2b0Cb6250d2887EF',
    name: 'Basic Attention Token',
    symbol: 'BAT',
    logo: BATIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '3',
    name: 'Binance Coin',
    symbol: 'BNB',
    logo: BNBIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 8,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '4',
    name: 'Bitcoin',
    symbol: 'BTC',
    logo: BTCIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 8,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '5',
    name: 'Algorand',
    symbol: 'ALGO',
    logo: ALGOIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 8,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  },
  {
    contractAddress: '0xE41d2489571d322189246DaFA5ebDe1F4699F498',
    name: '0x',
    symbol: 'ZRX',
    logo: ZRXIconUrl,
    isErc20: true,
    isErc721: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: ''
  }
]
