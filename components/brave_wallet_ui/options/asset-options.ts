// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../constants/types'

// options
import { AllNetworksOption } from './network-filter-options'

// icons
import {
  ArbIcon,
  AuroraIcon,
  BaseIcon,
  CeloIcon,
  FtmIcon,
  NeonIcon,
  OpIcon,
  AVAXIcon,
  BNBIcon,
  BTCIcon,
  ETHIcon,
  FILECOINIcon,
  MATICIcon,
  SOLIcon,
  ZECIcon,
  CardanoIcon,
  PolkadotIcon,
} from '../assets/network_token_icons/network_token_icons'

export const getNetworkLogo = (chainId: string, symbol: string): string => {
  if (chainId === BraveWallet.AURORA_MAINNET_CHAIN_ID) return AuroraIcon
  if (chainId === BraveWallet.OPTIMISM_MAINNET_CHAIN_ID) return OpIcon
  if (chainId === BraveWallet.POLYGON_MAINNET_CHAIN_ID) return MATICIcon
  if (chainId === BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID) return BNBIcon
  if (chainId === BraveWallet.AVALANCHE_MAINNET_CHAIN_ID) return AVAXIcon
  if (chainId === BraveWallet.FANTOM_MAINNET_CHAIN_ID) return FtmIcon
  if (chainId === BraveWallet.CELO_MAINNET_CHAIN_ID) return CeloIcon
  if (chainId === BraveWallet.ARBITRUM_MAINNET_CHAIN_ID) return ArbIcon
  if (chainId === BraveWallet.NEON_EVM_MAINNET_CHAIN_ID) return NeonIcon
  if (chainId === BraveWallet.BASE_MAINNET_CHAIN_ID) return BaseIcon
  if (chainId === AllNetworksOption.chainId)
    return AllNetworksOption.iconUrls[0]

  switch (symbol.toUpperCase()) {
    case 'SOL':
      return SOLIcon
    case 'ETH':
      return ETHIcon
    case 'FIL':
      return FILECOINIcon
    case 'BTC':
      return BTCIcon
    case 'ZEC':
      return ZECIcon
    case 'ADA':
      return CardanoIcon
    case 'DOT':
      return PolkadotIcon
  }

  return ''
}

export const makeNativeAssetLogo = (symbol: string, chainId: string) => {
  return getNetworkLogo(
    symbol.toUpperCase() === 'ETH' ? BraveWallet.MAINNET_CHAIN_ID : chainId,
    symbol,
  )
}

