// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types/constants
import { SKIP_PRICE_LOOKUP_COINGECKO_ID } from '../common/constants/magics'
import {
  SpotPriceRegistry,
  BraveWallet,
  externalWalletProviders,
  SupportedTestNetworks
} from '../constants/types'

// Utils
import Amount from './amount'

export const getTokenPriceFromRegistry = (
  spotPriceRegistry: SpotPriceRegistry,
  token: Pick<
    BraveWallet.BlockchainToken,
    'symbol' | 'contractAddress' | 'chainId' | 'coingeckoId' | 'isShielded'
  >
): BraveWallet.AssetPrice | undefined => {
  return spotPriceRegistry[getPriceIdForToken(token)]
}

export const getTokenPriceAmountFromRegistry = (
  spotPriceRegistry: SpotPriceRegistry,
  token: Pick<
    BraveWallet.BlockchainToken,
    'symbol' | 'contractAddress' | 'chainId' | 'coingeckoId' | 'isShielded'
  >
) => {
  const value = getTokenPriceFromRegistry(spotPriceRegistry, token)
  return value ? new Amount(value.price) : Amount.zero()
}

export const computeFiatAmount = ({
  spotPriceRegistry,
  value,
  token
}: {
  spotPriceRegistry?: SpotPriceRegistry
  value: string
  token: Pick<
    BraveWallet.BlockchainToken,
    | 'symbol'
    | 'contractAddress'
    | 'coingeckoId'
    | 'chainId'
    | 'coin'
    | 'decimals'
    | 'isShielded'
  >
}): Amount => {
  if (!spotPriceRegistry && value === '0') {
    return Amount.zero()
  }

  if (!spotPriceRegistry || !value) {
    return Amount.empty()
  }

  const price = getTokenPriceFromRegistry(spotPriceRegistry, token)
  if (!price) {
    return Amount.zero()
  }

  return new Amount(value)
    .divideByDecimals(token.decimals) // Wei → ETH conversion
    .times(price.price)
}

export const computeFiatAmountToAssetValue = ({
  spotPriceRegistry,
  value,
  token
}: {
  spotPriceRegistry?: SpotPriceRegistry
  value: string
  token: Pick<
    BraveWallet.BlockchainToken,
    | 'symbol'
    | 'contractAddress'
    | 'coingeckoId'
    | 'chainId'
    | 'coin'
    | 'decimals'
    | 'isShielded'
  >
}): Amount => {
  if (!spotPriceRegistry || !value) {
    return Amount.empty()
  }

  const priceInfo = getTokenPriceFromRegistry(spotPriceRegistry, token)
  if (!priceInfo) {
    return Amount.zero()
  }

  return new Amount(value).div(priceInfo.price).times(1)
}

export const getPriceIdForToken = (
  token: Pick<
    BraveWallet.BlockchainToken,
    'contractAddress' | 'symbol' | 'coingeckoId' | 'chainId' | 'isShielded'
  >
) => {
  if (token?.coingeckoId) {
    return token.coingeckoId.toLowerCase()
  }

  // Skip price of testnet tokens
  if (SupportedTestNetworks.includes(token.chainId)) {
    return SKIP_PRICE_LOOKUP_COINGECKO_ID
  }

  const isEthereumNetwork = token.chainId === BraveWallet.MAINNET_CHAIN_ID
  if (
    (isEthereumNetwork || externalWalletProviders.includes(token.chainId)) &&
    token.contractAddress
  ) {
    return token.contractAddress.toLowerCase()
  }

  return token.symbol.toLowerCase()
}
