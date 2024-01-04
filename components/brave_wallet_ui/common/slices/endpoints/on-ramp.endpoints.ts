// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet, SupportedOnRampNetworks } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// Utils
import {
  getBatTokensFromList,
  getNativeTokensFromList,
  getUniqueAssets
} from '../../../utils/asset-utils'
import { addLogoToToken } from '../../async/lib'
import { mapLimit } from 'async'
import { handleEndpointError } from '../../../utils/api-utils'

export const onRampEndpoints = ({ query }: WalletApiEndpointBuilderParams) => {
  return {
    getOnRampAssets: query<
      {
        rampAssetOptions: BraveWallet.BlockchainToken[]
        sardineAssetOptions: BraveWallet.BlockchainToken[]
        transakAssetOptions: BraveWallet.BlockchainToken[]
        stripeAssetOptions: BraveWallet.BlockchainToken[]
        coinbaseAssetOptions: BraveWallet.BlockchainToken[]
        allAssetOptions: BraveWallet.BlockchainToken[]
      },
      void
    >({
      queryFn: async (_arg, _store, _extraOptions, baseQuery) => {
        try {
          const {
            data: { blockchainRegistry }
          } = baseQuery(undefined)
          const { kRamp, kSardine, kTransak, kStripe, kCoinbase } =
            BraveWallet.OnRampProvider

          const rampAssets = await mapLimit(
            SupportedOnRampNetworks,
            10,
            async (chainId: string) =>
              await blockchainRegistry.getBuyTokens(kRamp, chainId)
          )

          const sardineAssets = await mapLimit(
            SupportedOnRampNetworks,
            10,
            async (chainId: string) =>
              await blockchainRegistry.getBuyTokens(kSardine, chainId)
          )

          const transakAssets = await mapLimit(
            SupportedOnRampNetworks,
            10,
            async (chainId: string) =>
              await blockchainRegistry.getBuyTokens(kTransak, chainId)
          )
          const stripeAssets = await mapLimit(
            SupportedOnRampNetworks,
            10,
            async (chainId: string) =>
              await blockchainRegistry.getBuyTokens(kStripe, chainId)
          )

          const coinbaseAssets = await mapLimit(
            SupportedOnRampNetworks,
            10,
            async (chainId: string) =>
              await blockchainRegistry.getBuyTokens(kCoinbase, chainId)
          )

          // add token logos
          const rampAssetOptions: BraveWallet.BlockchainToken[] =
            await mapLimit(
              rampAssets.flatMap((p) => p.tokens),
              10,
              async (token: BraveWallet.BlockchainToken) =>
                await addLogoToToken(token)
            )

          const sardineAssetOptions: BraveWallet.BlockchainToken[] =
            await mapLimit(
              sardineAssets.flatMap((p) => p.tokens),
              10,
              async (token: BraveWallet.BlockchainToken) =>
                await addLogoToToken(token)
            )

          const transakAssetOptions: BraveWallet.BlockchainToken[] =
            await mapLimit(
              transakAssets.flatMap((p) => p.tokens),
              10,
              async (token: BraveWallet.BlockchainToken) =>
                await addLogoToToken(token)
            )

          const stripeAssetOptions: BraveWallet.BlockchainToken[] =
            await mapLimit(
              stripeAssets.flatMap((p) => p.tokens),
              10,
              async (token: BraveWallet.BlockchainToken) =>
                await addLogoToToken(token)
            )

          const coinbaseAssetOptions: BraveWallet.BlockchainToken[] =
            await mapLimit(
              coinbaseAssets.flatMap((p) => p.tokens),
              10,
              async (token: BraveWallet.BlockchainToken) =>
                await addLogoToToken(token)
            )

          // separate native assets from tokens
          const {
            tokens: rampTokenOptions,
            nativeAssets: rampNativeAssetOptions
          } = getNativeTokensFromList(rampAssetOptions)

          const {
            tokens: sardineTokenOptions,
            nativeAssets: sardineNativeAssetOptions
          } = getNativeTokensFromList(sardineAssetOptions)

          const {
            tokens: transakTokenOptions,
            nativeAssets: transakNativeAssetOptions
          } = getNativeTokensFromList(transakAssetOptions)

          const {
            tokens: stripeTokenOptions,
            nativeAssets: stripeNativeAssetOptions
          } = getNativeTokensFromList(stripeAssetOptions)

          const {
            tokens: coinbaseTokenOptions,
            nativeAssets: coinbaseNativeAssetOptions
          } = getNativeTokensFromList(coinbaseAssetOptions)

          // separate BAT from other tokens
          const { bat: rampBatTokens, nonBat: rampNonBatTokens } =
            getBatTokensFromList(rampTokenOptions)

          const { bat: sardineBatTokens, nonBat: sardineNonBatTokens } =
            getBatTokensFromList(sardineTokenOptions)

          const { bat: transakBatTokens, nonBat: transakNonBatTokens } =
            getBatTokensFromList(transakTokenOptions)

          const { bat: stripeBatTokens, nonBat: stripeNonBatTokens } =
            getBatTokensFromList(stripeTokenOptions)

          const { bat: coinbaseBatTokens, nonBat: coinbaseNonBatTokens } =
            getBatTokensFromList(coinbaseTokenOptions)

          // sort lists
          // Move Gas coins and BAT to front of list
          const sortedRampOptions = [
            ...rampNativeAssetOptions,
            ...rampBatTokens,
            ...rampNonBatTokens
          ]
          const sortedSardineOptions = [
            ...sardineNativeAssetOptions,
            ...sardineBatTokens,
            ...sardineNonBatTokens
          ]
          const sortedTransakOptions = [
            ...transakNativeAssetOptions,
            ...transakBatTokens,
            ...transakNonBatTokens
          ]
          const sortedStripeOptions = [
            ...stripeNativeAssetOptions,
            ...stripeBatTokens,
            ...stripeNonBatTokens
          ]

          const sortedCoinbaseOptions = [
            ...coinbaseNativeAssetOptions,
            ...coinbaseBatTokens,
            ...coinbaseNonBatTokens
          ]

          const results = {
            rampAssetOptions: sortedRampOptions,
            sardineAssetOptions: sortedSardineOptions,
            transakAssetOptions: sortedTransakOptions,
            stripeAssetOptions: sortedStripeOptions,
            coinbaseAssetOptions: sortedCoinbaseOptions,
            allAssetOptions: getUniqueAssets([
              ...sortedRampOptions,
              ...sortedSardineOptions,
              ...sortedTransakOptions,
              ...sortedStripeOptions
            ])
          }

          return {
            data: results
          }
        } catch (error) {
          const errorMessage = `Unable to fetch onRamp assets: ${error}`
          console.log(errorMessage)
          return {
            error: errorMessage
          }
        }
      },
      providesTags: (_results, error, _arg) => {
        if (error) {
          return ['UNKNOWN_ERROR']
        }
        return ['OnRampAssets']
      }
    }),

    getOnRampFiatCurrencies: query<BraveWallet.OnRampCurrency[], void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { currencies } =
            await api.blockchainRegistry.getOnRampCurrencies()
          if (!currencies.length) {
            throw new Error('No currencies found')
          }
          return {
            data: currencies
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to fetch on-ramp fiat currencies',
            error
          )
        }
      }
    }),

    getBuyUrl: query<
      string,
      {
        onRampProvider: BraveWallet.OnRampProvider
        chainId: string
        address: string
        assetSymbol: string
        amount: string
        currencyCode: string
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { url, error } = await api.assetRatioService.getBuyUrlV1(
            arg.onRampProvider,
            arg.chainId,
            arg.address,
            arg.assetSymbol,
            arg.amount,
            arg.currencyCode
          )

          if (error) {
            throw new Error(error)
          }

          return {
            data: url
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to get ${getRampProviderName(
              arg.onRampProvider
            )} buy URL for: ${JSON.stringify(arg, undefined, 2)}`,
            error
          )
        }
      }
    })
  }
}

// internals
function getRampProviderName(onRampProvider: BraveWallet.OnRampProvider) {
  return Object.keys(BraveWallet.OnRampProvider)
    .find((key) => BraveWallet.OnRampProvider[key] === onRampProvider)
    ?.substring(1)
}
