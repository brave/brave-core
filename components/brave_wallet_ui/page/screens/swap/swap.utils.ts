// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet, SpotPriceRegistry } from '../../../constants/types'
import { LiquiditySource, QuoteOption, RouteTagsType } from './constants/types'

// Constants
import { LPMetadata } from './constants/metadata'

// Utils
import Amount from '../../../utils/amount'
import { getTokenPriceAmountFromRegistry } from '../../../utils/pricing-utils'
import { makeNetworkAsset } from '../../../options/asset-options'

function ensureUnique<T>(array: T[], key: keyof T): T[] {
  return [...new Set(array.map(a => a[key]))]
              .map(k => array.find(a => a[key] === k)!) as T[];
}

function getZeroExNetworkFee({
  quote,
  fromNetwork
}: {
  quote: BraveWallet.ZeroExQuote
  fromNetwork: BraveWallet.NetworkInfo
}): Amount {
  if (!fromNetwork) {
    return Amount.empty()
  }

  return new Amount(quote.gasPrice)
    .times(quote.gas)
    .divideByDecimals(fromNetwork.decimals)
}

export function getZeroExFromAmount({
  quote,
  fromToken
}: {
  quote: BraveWallet.ZeroExQuote
  fromToken: BraveWallet.BlockchainToken
}): Amount {
  return new Amount(quote.sellAmount).divideByDecimals(fromToken.decimals)
}

export function getZeroExToAmount({
  quote,
  toToken
}: {
  quote: BraveWallet.ZeroExQuote
  toToken: BraveWallet.BlockchainToken
}): Amount {
  return new Amount(quote.buyAmount).divideByDecimals(toToken.decimals)
}

export function getZeroExQuoteOptions({
  quote,
  fromNetwork,
  fromToken,
  toToken,
  spotPrices,
  defaultFiatCurrency
}: {
  quote: BraveWallet.ZeroExQuote
  fromNetwork: BraveWallet.NetworkInfo
  fromToken: BraveWallet.BlockchainToken
  toToken: BraveWallet.BlockchainToken
  spotPrices: SpotPriceRegistry
  defaultFiatCurrency: string
}): QuoteOption[] {
  const networkFee = getZeroExNetworkFee({ quote, fromNetwork })

  return [
    {
      fromAmount: new Amount(quote.sellAmount).divideByDecimals(
        fromToken.decimals
      ),
      toAmount: getZeroExToAmount({ quote, toToken }),
      minimumToAmount: undefined,
      fromToken,
      toToken,
      rate: new Amount(quote.buyAmount)
        .divideByDecimals(toToken.decimals)
        .div(new Amount(quote.sellAmount).divideByDecimals(fromToken.decimals)),
      impact: new Amount(quote.estimatedPriceImpact),
      sources: ensureUnique(quote.sources, 'name')
        .map((source) => ({
          name: source.name,
          proportion: new Amount(source.proportion)
        }))
        .filter((source) => source.proportion.gt(0)),
      routing: 'split', // 0x supports split routing only
      networkFee,
      networkFeeFiat: networkFee.isUndefined()
        ? ''
        : networkFee
            .times(
              getTokenPriceAmountFromRegistry(
                spotPrices,
                makeNetworkAsset(fromNetwork)
              )
            )
            .formatAsFiat(defaultFiatCurrency),
      provider: BraveWallet.SwapProvider.kZeroEx,
      // There is only 1 quote returned for Ox
      // making it the Fastest and Cheapest.
      tags: ['FASTEST', 'CHEAPEST']
    }
  ]
}

function getJupiterNetworkFee({
  quote,
  fromNetwork
}: {
  quote: BraveWallet.JupiterQuote
  fromNetwork: BraveWallet.NetworkInfo
}): Amount {
  if (!fromNetwork) {
    return Amount.empty()
  }

  return new Amount('0.000005')
}

export function getJupiterFromAmount({
  quote,
  fromToken
}: {
  quote: BraveWallet.JupiterQuote
  fromToken: BraveWallet.BlockchainToken
}): Amount {
  return new Amount(quote.inAmount).divideByDecimals(fromToken.decimals)
}

export function getJupiterToAmount({
  quote,
  toToken
}: {
  quote: BraveWallet.JupiterQuote
  toToken: BraveWallet.BlockchainToken
}): Amount {
  return new Amount(quote.outAmount).divideByDecimals(toToken.decimals)
}

export function getJupiterQuoteOptions({
  quote,
  fromNetwork,
  fromToken,
  toToken,
  spotPrices,
  defaultFiatCurrency
}: {
  quote: BraveWallet.JupiterQuote
  fromNetwork: BraveWallet.NetworkInfo
  fromToken: BraveWallet.BlockchainToken
  toToken: BraveWallet.BlockchainToken
  spotPrices: SpotPriceRegistry
  defaultFiatCurrency: string
}): QuoteOption[] {
  const networkFee = getJupiterNetworkFee({ quote, fromNetwork })

  return [
    {
      fromAmount: new Amount(quote.inAmount).divideByDecimals(
        fromToken.decimals
      ),
      toAmount: getJupiterToAmount({ quote, toToken }),
      // TODO: minimumToAmount is applicable only for ExactIn swapMode.
      // Create a maximumFromAmount field for ExactOut swapMode if needed.
      minimumToAmount: new Amount(quote.otherAmountThreshold).divideByDecimals(
        toToken.decimals
      ),
      fromToken,
      toToken,
      rate: new Amount(quote.outAmount)
        .divideByDecimals(toToken.decimals)
        .div(new Amount(quote.inAmount).divideByDecimals(fromToken.decimals)),
      impact: new Amount(quote.priceImpactPct),
      sources: quote.routePlan.map((step) => ({
        name: step.swapInfo.label,
        proportion: new Amount(step.percent).times(0.01)
      })),
      routing: 'flow',
      networkFee,
      networkFeeFiat: networkFee.isUndefined()
        ? ''
        : networkFee
            .times(
              getTokenPriceAmountFromRegistry(
                spotPrices,
                makeNetworkAsset(fromNetwork)
              )
            )
            .formatAsFiat(defaultFiatCurrency),
      provider: BraveWallet.SwapProvider.kJupiter,
      // There is only 1 quote returned for Jupiter
      // making it the Fastest and Cheapest.
      tags: ['FASTEST', 'CHEAPEST']
    }
  ]
}

// LiFi

export function getLiFiFromAmount(route: BraveWallet.LiFiRoute): Amount {
  return new Amount(route.fromAmount).divideByDecimals(route.fromToken.decimals)
}

export function getLiFiToAmount(route: BraveWallet.LiFiRoute): Amount {
  return new Amount(route.toAmount).divideByDecimals(route.toToken.decimals)
}

export function getLiFiQuoteOptions({
  quote,
  fromNetwork,
  fromToken,
  toToken,
  spotPrices,
  defaultFiatCurrency
}: {
  quote: BraveWallet.LiFiQuote
  fromNetwork: BraveWallet.NetworkInfo
  spotPrices: SpotPriceRegistry
  defaultFiatCurrency: string
  fromToken: BraveWallet.BlockchainToken
  toToken: BraveWallet.BlockchainToken
}): QuoteOption[] {
  return quote.routes.map((route) => {
    const networkFee = route.steps[0].estimate.gasCosts
      .reduce((total, cost) => {
        return total.plus(cost.amount)
      }, new Amount('0'))
      .divideByDecimals(fromNetwork.decimals)

    const fromAmount = new Amount(route.fromAmount).divideByDecimals(
      fromToken.decimals
    )

    const toAmount = new Amount(route.toAmount).divideByDecimals(
      route.toToken.decimals
    )

    const fromAmountFiat = fromAmount.times(
      getTokenPriceAmountFromRegistry(spotPrices, fromToken)
    )

    const toAmountFiat = toAmount.times(
      getTokenPriceAmountFromRegistry(spotPrices, toToken)
    )

    const fiatDiff = toAmountFiat.minus(fromAmountFiat)
    const fiatDiffRatio = fiatDiff.div(fromAmountFiat)
    const impact = fiatDiffRatio.times(100).toAbsoluteValue()

    return {
      fromAmount: fromAmount,
      fromToken: fromToken,
      impact,
      minimumToAmount: new Amount(route.toAmountMin).divideByDecimals(
        toToken.decimals
      ),
      networkFee,
      networkFeeFiat: networkFee.isUndefined()
        ? ''
        : networkFee
            .times(
              getTokenPriceAmountFromRegistry(
                spotPrices,
                makeNetworkAsset(fromNetwork)
              )
            )
            .formatAsFiat(defaultFiatCurrency),
      rate: toAmount.div(fromAmount),
      routing: 'flow',
      sources: route.steps.map((step) => ({
        name: step.toolDetails.name,
        proportion: new Amount(1),
        logo: step.toolDetails.logo,
        tool: step.tool,
        includedSteps: step.includedSteps
      })),
      toAmount: toAmount,
      toToken: toToken,
      executionDuration: route.steps
        .map((step) => Number(step.estimate.executionDuration))
        .reduce((a, b) => a + b, 0)
        .toString(),
      provider: BraveWallet.SwapProvider.kLiFi,
      tags: [
        ...new Set(route.tags)
      ].filter((tag) =>
        ['CHEAPEST', 'FASTEST'].includes(tag)
      ) as RouteTagsType[],
      id: route.uniqueId
    }
  })
}

// Squid

export function getSquidFromAmount({
  quote,
  fromToken
}: {
  quote: BraveWallet.SquidQuote
  fromToken: BraveWallet.BlockchainToken
}): Amount {
  return new Amount(quote.fromAmount).divideByDecimals(fromToken.decimals)
}

export function getSquidToAmount({
  quote,
  toToken
}: {
  quote: BraveWallet.SquidQuote
  toToken: BraveWallet.BlockchainToken
}): Amount {
  return new Amount(quote.toAmount).divideByDecimals(toToken.decimals)
}

export function getSquidQuoteOptions({
  quote,
  fromNetwork,
  spotPrices,
  defaultFiatCurrency
}: {
  quote: BraveWallet.SquidQuote
  fromNetwork: BraveWallet.NetworkInfo
  spotPrices: SpotPriceRegistry
  defaultFiatCurrency: string
}): QuoteOption[] {
  const networkFee = quote.gasCosts
    .filter((e) => e.token.chainId === fromNetwork.chainId)
    .reduce(
      (total, cost) =>
        total.plus(
          new Amount(cost.amount).divideByDecimals(cost.token.decimals)
        ),
      Amount.zero()
    )

  const fromAmount = new Amount(quote.fromAmount).divideByDecimals(
    quote.fromToken.decimals
  )

  const toAmount = new Amount(quote.toAmount).divideByDecimals(
    quote.toToken.decimals
  )

  return [
    {
      fromAmount,
      toAmount,
      minimumToAmount: new Amount(quote.toAmountMin).divideByDecimals(
        quote.toToken.decimals
      ),
      fromToken: quote.fromToken,
      toToken: quote.toToken,
      rate: toAmount.div(fromAmount),
      impact: new Amount(quote.aggregatePriceImpact).times(100),
      sources: ensureUnique(quote.actions, 'provider')
        .map((action) => ({
          name: action.provider,
          proportion: new Amount(1),
          logo: action.logoUri
        })),
      routing: 'flow',
      networkFee,
      networkFeeFiat: networkFee.isUndefined()
        ? ''
        : networkFee
            .times(
              getTokenPriceAmountFromRegistry(
                spotPrices,
                makeNetworkAsset(fromNetwork)
              )
            )
            .formatAsFiat(defaultFiatCurrency),
      provider: BraveWallet.SwapProvider.kSquid,
      tags: ['CHEAPEST', 'FASTEST']
    }
  ]
}

export const getLPIcon = (source: Pick<LiquiditySource, 'name' | 'logo'>) => {
  const iconFromMetadata = LPMetadata[source.name]
  if (iconFromMetadata) {
    return iconFromMetadata
  }
  if (source.logo) {
    return `chrome://image?${source.logo}`
  }
  return ''
}
