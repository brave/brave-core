// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet } from '../../../constants/types'
import { LiquiditySource, QuoteOption, RouteTagsType } from './constants/types'

// Constants
import { LPMetadata } from './constants/metadata'

// Utils
import Amount from '../../../utils/amount'
import { getTokenPriceAmountFromRegistry } from '../../../utils/pricing-utils'
import { makeNetworkAsset } from '../../../options/asset-options'
import { sanitizeImageURL } from '../../../utils/string-utils'

// Gate3

export function getGate3FromAmount({
  route,
  fromToken,
}: {
  route: BraveWallet.Gate3SwapRoute
  fromToken: BraveWallet.BlockchainToken
}): Amount {
  return new Amount(route.sourceAmount).divideByDecimals(fromToken.decimals)
}

export function getGate3ToAmount({
  route,
  toToken,
}: {
  route: BraveWallet.Gate3SwapRoute
  toToken: BraveWallet.BlockchainToken
}): Amount {
  return new Amount(route.destinationAmount).divideByDecimals(toToken.decimals)
}

export function getGate3QuoteOptions({
  quote,
  fromToken,
  toToken,
  fromNetwork,
  spotPrices,
  defaultFiatCurrency,
}: {
  quote: BraveWallet.Gate3SwapQuote
  fromToken: BraveWallet.BlockchainToken
  toToken: BraveWallet.BlockchainToken
  fromNetwork: BraveWallet.NetworkInfo
  spotPrices: BraveWallet.AssetPrice[]
  defaultFiatCurrency: string
}): QuoteOption[] {
  return quote.routes.map((route) => {
    const fromAmount = new Amount(route.sourceAmount).divideByDecimals(
      fromToken.decimals,
    )

    const toAmount = new Amount(route.destinationAmount).divideByDecimals(
      toToken.decimals,
    )

    const minimumToAmount = new Amount(
      route.destinationAmountMin,
    ).divideByDecimals(toToken.decimals)

    const fromAmountFiat = fromAmount.times(
      getTokenPriceAmountFromRegistry(spotPrices, fromToken),
    )

    const toAmountFiat = toAmount.times(
      getTokenPriceAmountFromRegistry(spotPrices, toToken),
    )

    const fiatDiff = toAmountFiat.minus(fromAmountFiat)
    const fiatDiffRatio = fiatDiff.div(fromAmountFiat)
    const impact = route.priceImpact
      ? new Amount(route.priceImpact).toAbsoluteValue()
      : fiatDiffRatio.times(100).toAbsoluteValue()

    // Use network fee from route if available, otherwise zero
    // If gasless is true, network fees are sponsored/waived
    const networkFee =
      route.gasless || !route.networkFee
        ? Amount.zero()
        : new Amount(route.networkFee.amount).divideByDecimals(
            route.networkFee.decimals,
          )

    return {
      fromAmount,
      toAmount,
      minimumToAmount,
      fromToken,
      toToken,
      rate: toAmount.div(fromAmount),
      impact,
      sources: [],
      steps: [...route.steps].reverse(),
      routing: 'flow',
      networkFee,
      networkFeeFiat: networkFee.isUndefined()
        ? ''
        : networkFee
            .times(
              getTokenPriceAmountFromRegistry(
                spotPrices,
                makeNetworkAsset(fromNetwork),
              ),
            )
            .formatAsFiat(defaultFiatCurrency),
      provider: route.provider,
      executionDuration: route.estimatedTime ?? undefined,
      tags: ['CHEAPEST', 'FASTEST'] as RouteTagsType[],
      id: route.id,
    }
  })
}

export const getLPIcon = (source: Pick<LiquiditySource, 'name' | 'logo'>) => {
  const iconFromMetadata = LPMetadata[source.name]
  if (iconFromMetadata) {
    return iconFromMetadata
  }
  if (source.logo) {
    return sanitizeImageURL(source.logo)
  }
  return ''
}
