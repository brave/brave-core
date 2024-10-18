// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  MeldCryptoCurrency,
  SpotPriceRegistry
} from '../constants/types'

export const getAssetSymbol = (asset: MeldCryptoCurrency) => {
  return asset.currencyCode.replace(`_${asset.chainCode}`, '')
}

export const getAssetPriceId = (asset: MeldCryptoCurrency) => {
  const isEthereumNetwork =
    asset.chainId?.toLowerCase() === BraveWallet.MAINNET_CHAIN_ID.toLowerCase()
  if (isEthereumNetwork && asset.contractAddress) {
    return asset.contractAddress.toLowerCase()
  }

  return getAssetSymbol(asset)?.toLowerCase() ?? ''
}

export const getTokenPriceFromRegistry = (
  spotPriceRegistry: SpotPriceRegistry,
  asset: MeldCryptoCurrency
): BraveWallet.AssetPrice | undefined => {
  return spotPriceRegistry[getAssetPriceId(asset)]
}

export const getAssetIdKey = (asset: MeldCryptoCurrency) => {
  return `0x${parseInt(asset.chainId ?? '').toString(16)}-${
    asset.currencyCode
  }-${asset.contractAddress}`
}

export const getMeldTokensCoinType = (
  asset: Pick<MeldCryptoCurrency, 'chainCode'>
) => {
  switch (asset.chainCode) {
    case 'BTC':
      return BraveWallet.CoinType.BTC
    case 'FIL':
      return BraveWallet.CoinType.FIL
    case 'ZEC':
      return BraveWallet.CoinType.ZEC
    case 'SOLANA':
      return BraveWallet.CoinType.SOL
    default:
      return BraveWallet.CoinType.ETH
  }
}

export const getMeldTokensChainId = (
  asset: Pick<MeldCryptoCurrency, 'chainId' | 'chainCode'>
) => {
  switch (asset.chainCode) {
    case 'BTC':
      return BraveWallet.BITCOIN_MAINNET
    case 'FIL':
      return BraveWallet.FILECOIN_MAINNET
    case 'ZEC':
      return BraveWallet.Z_CASH_MAINNET
    default:
      return asset.chainId
  }
}
