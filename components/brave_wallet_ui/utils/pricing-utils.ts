// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { AssetPriceWithContractAndChainId, SupportedTestNetworks } from '../constants/types'
import Amount from './amount'

export const findAssetPrice = (
  spotPrices: AssetPriceWithContractAndChainId[],
  symbol: string,
  contractAddress: string,
  chainId: string
) => {
  if (SupportedTestNetworks.includes(chainId)) {
    return spotPrices.find(
      (token) => token.fromAsset.toLowerCase() === symbol.toLowerCase() &&
        token.contractAddress === contractAddress
    )?.price ?? ''
  }
  return spotPrices.find(
    (token) => token.fromAsset.toLowerCase() === symbol.toLowerCase() &&
      token.contractAddress === contractAddress &&
      token.chainId === chainId
  )?.price ?? ''
}

export const computeFiatAmount = (
  spotPrices: AssetPriceWithContractAndChainId[],
  asset: {
    value: string
    symbol: string
    decimals: number
    contractAddress: string
    chainId: string
  }
): Amount => {
  const price = findAssetPrice(spotPrices, asset.symbol, asset.contractAddress, asset.chainId)

  if (!price || !asset.value) {
    return Amount.empty()
  }

  return new Amount(asset.value)
    .divideByDecimals(asset.decimals) // Wei → ETH conversion
    .times(price)
}

export const computeFiatAmountToAssetValue = (
  fiatAmount: string,
  spotPrices: AssetPriceWithContractAndChainId[],
  assetSymbol: string,
  contractAddress: string,
  chainId: string
): Amount => {
  const price = findAssetPrice(spotPrices, assetSymbol, contractAddress, chainId)

  if (!price || !fiatAmount) {
    return Amount.empty()
  }

  return new Amount(fiatAmount).div(price).times(1)
}
