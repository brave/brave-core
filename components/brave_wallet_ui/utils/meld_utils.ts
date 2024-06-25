// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  MeldCryptoCurrency,
  SpotPriceRegistry
} from '../constants/types'

export const isNativeAsset = (asset: MeldCryptoCurrency) =>
  !asset.contractAddress

export const getAssetName = (asset: MeldCryptoCurrency) => {
  return isNativeAsset(asset) ? asset.chainName : asset.name
}

export const getAssetSymbol = (asset: MeldCryptoCurrency) => {
  return isNativeAsset(asset)
    ? asset.chainCode ?? ''
    : asset.currencyCode.replace(`_${asset.chainCode}`, '')
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

export const getAssetIdKey = (
  asset: MeldCryptoCurrency
) => {
  return `${asset.chainId}-${asset.currencyCode}-${asset.contractAddress}`
}
