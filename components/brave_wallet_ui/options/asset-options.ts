// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,I
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'
import {
  BNBIconUrl,
  ETHIconUrl,
  SOLIconUrl,
  FILECOINIconUrl
} from '../assets/asset-icons'
import {
  AuroraIcon,
  CeloIcon,
  FantomIcon,
  OptimismIcon
} from '../assets/network-icons'
import { AllNetworksOption } from './network-filter-options'

export const makeNetworkAsset = (network: BraveWallet.NetworkInfo): BraveWallet.BlockchainToken => {
  let logo
  switch (true) {
    case network.chainId === BraveWallet.AURORA_MAINNET_CHAIN_ID:
      logo = AuroraIcon
      break

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

    case network.chainId === AllNetworksOption.chainId:
      logo = AllNetworksOption.iconUrls[0]
      break

    case network.symbol.toUpperCase() === 'SOL':
      logo = SOLIconUrl
      break

    case network.symbol.toUpperCase() === 'FIL':
      logo = FILECOINIconUrl
      break

    case network.symbol.toUpperCase() === 'ETH':
      logo = 'chrome://erc-token-images/eth.png'
      break

    default:
      logo = ''
  }

  return {
    contractAddress: '',
    name: network.symbolName,
    symbol: network.symbol,
    logo: logo,
    isErc20: false,
    isErc721: false,
    decimals: network.decimals,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    chainId: network.chainId,
    coin: network.coin
  }
}

export const ETH = {
  contractAddress: '',
  name: 'Ethereum',
  symbol: 'ETH',
  logo: ETHIconUrl,
  isErc20: false,
  isErc721: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  chainId: '0x1',
  coin: BraveWallet.CoinType.ETH
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
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH
} as BraveWallet.BlockchainToken
