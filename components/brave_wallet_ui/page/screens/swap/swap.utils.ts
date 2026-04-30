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
  const toTokenPrice = getTokenPriceAmountFromRegistry(spotPrices, toToken)
  const networkAssetPrice = getTokenPriceAmountFromRegistry(
    spotPrices,
    makeNetworkAsset(fromNetwork),
  )

  const options: QuoteOption[] = []
  const netValues: Amount[] = []
  const durations: (number | undefined)[] = []
  let bestNetValue: Amount | undefined
  let bestDuration: number | undefined

  for (const route of quote.routes) {
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

    const toAmountFiat = toAmount.times(toTokenPrice)

    const fiatDiff = toAmountFiat.minus(fromAmountFiat)
    const fiatDiffRatio = fiatDiff.div(fromAmountFiat)
    const impact = route.priceImpact
      ? new Amount(route.priceImpact).toAbsoluteValue()
      : fiatDiffRatio.times(100).toAbsoluteValue()

    // gasless routes have their network fee sponsored by the relayer.
    const networkFee =
      route.gasless || !route.networkFee
        ? Amount.zero()
        : new Amount(route.networkFee.amount).divideByDecimals(
            route.networkFee.decimals,
          )

    const networkFeeFiat = networkFee.times(networkAssetPrice)
    const netValueFiat = toAmountFiat.minus(networkFeeFiat)

    const parsedDuration =
      route.estimatedTime !== undefined
        ? Number(route.estimatedTime)
        : undefined
    const duration =
      parsedDuration !== undefined && Number.isFinite(parsedDuration)
        ? parsedDuration
        : undefined

    options.push({
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
        : networkFeeFiat.formatAsFiat(defaultFiatCurrency),
      provider: route.provider,
      executionDuration: route.estimatedTime ?? undefined,
      tags: [],
      id: route.id,
    })
    netValues.push(netValueFiat)
    durations.push(duration)

    if (bestNetValue === undefined || netValueFiat.gt(bestNetValue)) {
      bestNetValue = netValueFiat
    }
    if (
      duration !== undefined
      && (bestDuration === undefined || duration < bestDuration)
    ) {
      bestDuration = duration
    }
  }

  // Every route matching the best score is tagged so genuine ties surface
  // multiple badges instead of picking an arbitrary winner.
  for (let i = 0; i < options.length; i++) {
    const tags: RouteTagsType[] = []
    if (bestNetValue !== undefined && netValues[i].eq(bestNetValue)) {
      tags.push('CHEAPEST')
    }
    if (bestDuration !== undefined && durations[i] === bestDuration) {
      tags.push('FASTEST')
    }
    options[i].tags = tags
  }

  return options
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
