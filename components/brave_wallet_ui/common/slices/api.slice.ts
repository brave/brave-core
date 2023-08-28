// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { batch } from 'react-redux'
import { EntityId, Store } from '@reduxjs/toolkit'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { PatchCollection } from '@reduxjs/toolkit/dist/query/core/buildThunks'
import { mapLimit } from 'async'

// types
import { WalletPanelApiProxy } from '../../panel/wallet_panel_api_proxy'
import {
  ApproveERC20Params,
  BraveWallet,
  CoinTypes,
  ER20TransferParams,
  ERC721Metadata,
  ERC721TransferFromParams,
  ETHFilForwarderTransferFromParams,
  SendEthTransactionParams,
  SendFilTransactionParams,
  SendSolTransactionParams,
  SerializableTransactionInfo,
  SPLTransferFromParams,
  SupportedCoinTypes,
  SpotPriceRegistry
} from '../../constants/types'
import {
  CancelTransactionPayload,
  IsEip1559Changed,
  RetryTransactionPayload,
  SetUserAssetVisiblePayloadType,
  SpeedupTransactionPayload,
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionNonceType,
  UpdateUnapprovedTransactionSpendAllowanceType
} from '../constants/action_types'
import { PanelActions } from '../../panel/actions'

// entities
import {
  networkEntityAdapter,
  NetworksRegistry,
  selectSwapSupportedNetworksFromQueryResult,
  selectMainnetNetworksFromQueryResult,
  selectAllNetworksFromQueryResult,
  selectOffRampNetworksFromQueryResult,
  selectOnRampNetworksFromQueryResult,
  selectVisibleNetworksFromQueryResult
} from './entities/network.entity'
import {
  AccountInfoEntityState,
  accountInfoEntityAdaptor,
  AccountInfoEntity
} from './entities/account-info.entity'
import {
  blockchainTokenEntityAdaptor,
  blockchainTokenEntityAdaptorInitialState,
  BlockchainTokenEntityAdaptorState
} from './entities/blockchain-token.entity'
import { TokenBalancesForChainId } from './entities/token-balance.entity'

// api
import { apiProxyFetcher } from '../async/base-query-cache'
import { createWalletApiBase } from './api-base.slice'
import {
  transactionSimulationEndpoints
} from './endpoints/tx-simulation.endpoints'
import { braveRewardsApiEndpoints } from './endpoints/rewards.endpoints'

// utils
import { getAccountType } from '../../utils/account-utils'
import { cacher, TX_CACHE_TAGS } from '../../utils/query-cache-utils'
import type WalletApiProxy from '../wallet_api_proxy'
import {
  addChainIdToToken,
  getAssetIdKey,
  GetBlockchainTokenIdArg,
  isNativeAsset
} from '../../utils/asset-utils'
import { getEntitiesListFromEntityState } from '../../utils/entities.utils'
import {
  getCoinFromTxDataUnion,
  hasEIP1559Support
} from '../../utils/network-utils'
import Amount from '../../utils/amount'
import { shouldReportTransactionP3A, sortTransactionByDate, toTxDataUnion } from '../../utils/tx-utils'
import {
  makeSerializableTransaction
} from '../../utils/model-serialization-utils'
import { addLogoToToken } from '../async/lib'
import {
  dialogErrorFromLedgerErrorCode,
  signLedgerEthereumTransaction,
  signLedgerFilecoinTransaction,
  signLedgerSolanaTransaction,
  signTrezorTransaction
} from '../async/hardware'

import {
  maxBatchSizePrice,
  maxConcurrentPriceRequests
} from './constants'

type GetAccountTokenCurrentBalanceArg = {
  coin: BraveWallet.CoinType,
  address: string,
  token: GetBlockchainTokenIdArg
}

type GetCombinedTokenBalanceForAllAccountsArg =
  GetAccountTokenCurrentBalanceArg['token'] &
    Pick<BraveWallet.BlockchainToken, 'coin'>

type GetSPLTokenBalancesArg = {
  pubkey: string
  chainId: string

  /**
   * optional, if not provided, will fetch all tokens using
   * getTokenAccountsByOwner.
   */
  tokens?: GetBlockchainTokenIdArg[]
  coin: typeof CoinTypes.SOL
}
type GetERC20TokenBalancesArg = {
  address: string
  tokens: GetBlockchainTokenIdArg[]
  chainId: string
  coin: typeof CoinTypes.ETH
}
type GetTokenBalancesForChainIdArg =
  | GetSPLTokenBalancesArg
  | GetERC20TokenBalancesArg

export interface IsEip1559ChangedMutationArg {
  id: string
  isEip1559: boolean
}

interface GetFVMAddressArg {
  coin: BraveWallet.CoinType | undefined
  isMainNet: boolean
  addresses: string[]
}

type GetFVMAddressResult = Map<string, {address: string, fvmAddress: string}>

interface GetTransactionsQueryArg {
  /**
   * will fetch for all account addresses if null
  */
  coinType: BraveWallet.CoinType | null
  /**
   * will fetch for all coin-type addresses if null
  */
  address: string | null
  /**
   * will fetch for all coin-type chains if null
  */
  chainId: string | null
}

interface GetTokenSpotPricesArg {
  ids: string[]
  timeframe?: BraveWallet.AssetPriceTimeframe
}

const NETWORK_TAG_IDS = {
  DEFAULTS: 'DEFAULTS',
  HIDDEN: 'HIDDEN',
  LIST: 'LIST',
  MAINNETS: 'MAINNETS',
  OFF_RAMPS: 'OFF_RAMP',
  ON_RAMPS: 'ON_RAMP',
  REGISTRY: 'REGISTRY',
  SELECTED: 'SELECTED',
  SWAP_SUPPORTED: 'SWAP_SUPPORTED',
  VISIBLE: 'VISIBLE',
} as const

const ACCOUNT_TAG_IDS = {
  REGISTRY: 'REGISTRY',
  SELECTED: 'SELECTED',
}

const TOKEN_TAG_IDS = {
  REGISTRY: 'REGISTRY',
} as const

export function createWalletApi () {
  // base to add endpoints to
  return createWalletApiBase().injectEndpoints({
    endpoints: ({ mutation, query }) => { return {
      //
      // Account Info
      //
      getAccountInfosRegistry: query<AccountInfoEntityState, void>({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          const { cache } = baseQuery(undefined)
          return {
            data: await cache.getAccountsRegistry()
          }
        },
        providesTags: (res, err) =>
          err
            ? ['UNKNOWN_ERROR']
            : [{ type: 'AccountInfos', id: ACCOUNT_TAG_IDS.REGISTRY }]
      }),
      setSelectedAccount: mutation<
        BraveWallet.AccountId,
        BraveWallet.AccountId
      >({
        queryFn: async (accountId, api, extraOptions, baseQuery) => {
          const {
            cache,
            // apiProxy
            data: { keyringService }
          } = baseQuery(undefined)

          await keyringService.setSelectedAccount(accountId)
          cache.clearSelectedAccount()

          return {
            data: accountId
          }
        },
        invalidatesTags: [
          { type: 'Network', id: NETWORK_TAG_IDS.SELECTED },
          { type: 'AccountInfos', id: ACCOUNT_TAG_IDS.SELECTED }
        ]
      }),
      getSelectedAccountAddress: query<string, void>({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          const { cache } = baseQuery(undefined)

          let selectedAddress: string | null =
            await cache.getSelectedAccountAddress()

          const accountsRegistry = await cache.getAccountsRegistry()

          if (
            // If the selected address is null, set the selected account address to the fallback address
            selectedAddress === null ||
            selectedAddress === '' ||
            // If a user has already created an wallet but then chooses to restore
            // a different wallet, getSelectedAccount still returns the previous wallets
            // selected account.
            // This check looks to see if the returned selectedAccount exist in the accountInfos
            // payload, if not it will setSelectedAccount to the fallback address
            !accountsRegistry.ids.find(
              (accountId) =>
                String(accountId).toLowerCase() ===
                selectedAddress?.toLowerCase()
            )
          ) {
            const fallbackAccount = accountsRegistry[accountsRegistry.ids[0]]
            await dispatch(
              walletApi.endpoints.setSelectedAccount.initiate(fallbackAccount)
            )
            return {
              data: fallbackAccount.address
            }
          }

          return {
            data: selectedAddress
          }
        },
        providesTags: [{ type: 'AccountInfos', id: ACCOUNT_TAG_IDS.SELECTED }]
      }),
      //
      // Default Currencies
      //
      getDefaultFiatCurrency: query<string, void>({
        queryFn: async (arg, api, extraOptions, baseQuery) => {
          try {
            const { braveWalletService } = baseQuery(undefined).data
            const { currency } =
              await braveWalletService.getDefaultBaseCurrency()
            const defaultFiatCurrency = currency.toLowerCase()
            return {
              data: defaultFiatCurrency
            }
          } catch (error) {
            return {
              error: 'Unable to fetch default fiat currency'
            }
          }
        },
        providesTags: ['DefaultFiatCurrency']
      }),
      setDefaultFiatCurrency: mutation<string, string>({
        queryFn: async (currencyArg, api, extraOptions, baseQuery) => {
          try {
            const { braveWalletService } = baseQuery(undefined).data
            braveWalletService.setDefaultBaseCurrency(currencyArg)
            return {
              data: currencyArg
            }
          } catch (error) {
            return {
              error: `Unable to set default fiat currency to ${currencyArg}`
            }
          }
        },
        invalidatesTags: ['DefaultFiatCurrency']
      }),
      //
      // Networks
      //
      getNetworksRegistry: query<NetworksRegistry, void>({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { cache } = baseQuery(undefined)
            return {
              data: await cache.getNetworksRegistry()
            }
          } catch (error) {
            console.error(error)
            return {
              error: `Unable to fetch AllNetworks ${error}`
            }
          }
        },
        providesTags: (res, err, arg) =>
          err
            ? ['UNKNOWN_ERROR']
            : [{ type: 'Network', id: NETWORK_TAG_IDS.REGISTRY }]
      }),
      getSwapSupportedNetworkIds: query<string[], void>({
        queryFn: async (arg, api, extraOptions, baseQuery) => {
          try {
            const {
              data: { swapService },
              cache
            } = baseQuery(undefined)

            const networksRegistry = await cache.getNetworksRegistry()

            const chainIdsWithSupportFlags = await mapLimit(
              networksRegistry.ids,
              10,
              async (chainId: string) => {
                const { result } = await swapService.isSwapSupported(
                  chainId.toString()
                )
                return {
                  chainId,
                  supported: result
                }
              }
            )

            const swapChainIds = chainIdsWithSupportFlags
              .filter(({ supported }) => !!supported)
              .map((net) => net.chainId.toString())

            return {
              data: swapChainIds
            }
          } catch (error) {
            console.error(error)
            return {
              error: `Error occurred within "getSwapSupportedNetworks": ${
                error.toString() //
              }`
            }
          }
        },
        providesTags: [{ type: 'Network', id: NETWORK_TAG_IDS.SWAP_SUPPORTED }]
      }),
      getSelectedChain: query<BraveWallet.NetworkInfo, void>({
        queryFn: async (arg, api, extraOptions, baseQuery) => {
          try {
            return {
              data: await getSelectedNetwork(baseQuery(undefined).data)
            }
          } catch (error) {
            console.error(error)
            return {
              error: `Unable to fetch the currently selected chain`
            }
          }
        },
        providesTags: (res, err) =>
          err
            ? ['UNKNOWN_ERROR']
            : [{ type: 'Network', id: NETWORK_TAG_IDS.SELECTED }]
      }),
      setSelectedCoin: mutation<BraveWallet.CoinType, BraveWallet.CoinType>({
        queryFn: (coinTypeArg, api, extraOptions, baseQuery) => {
          try {
            const { braveWalletService } = baseQuery(undefined).data
            braveWalletService.setSelectedCoin(coinTypeArg)
            return { data: coinTypeArg }
          } catch (error) {
            return {
              error: `Unable to mutate selectedCoin: ${error}`
            }
          }
        },
        invalidatesTags: [
          { type: 'Network', id: NETWORK_TAG_IDS.SELECTED },
          { type: 'AccountInfos', id: ACCOUNT_TAG_IDS.SELECTED }
        ]
      }),
      setNetwork: mutation<
        Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'> & {
          selectedAccount?: AccountInfoEntity
        },
        Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
      >({
        queryFn: async (
          { chainId, coin },
          { dispatch },
          extraOptions,
          baseQuery
        ) => {
          try {
            const {
              data: { braveWalletService },
              cache
            } = baseQuery(undefined)

            await dispatch(
              walletApi.endpoints.setSelectedCoin.initiate(coin)
            ).unwrap()

            const { success } =
              await braveWalletService.setChainIdForActiveOrigin(coin, chainId)
            if (!success) {
              throw new Error(
                'braveWalletService.SetChainIdForActiveOrigin failed'
              )
            }

            // FIXME(josheleonard): could be written in a more efficient way.
            // Clients setting networks must be aware of the selected account
            // corresponding to the new network.
            const accountsRegistry = await cache.getAccountsRegistry()
            cache.clearSelectedAccount()
            const selectedAccountAddress =
              await cache.getSelectedAccountAddress()

            const selectedAccount = selectedAccountAddress
              ? accountsRegistry.entities[
                  accountInfoEntityAdaptor.selectId({
                    address: selectedAccountAddress
                  })
                ]
              : undefined

            return {
              data: { chainId, coin, selectedAccount }
            }
          } catch (error) {
            console.error(error)
            return {
              error: `Unable to change selected network to: (chainId: ${
                chainId //
              }, coin: ${
                coin //
              })`
            }
          }
        },
        invalidatesTags: (result, error, { coin }) => [
          { type: 'Network', id: NETWORK_TAG_IDS.SELECTED },
          { type: 'Network', id: NETWORK_TAG_IDS.DEFAULTS },
          { type: 'AccountInfos', id: ACCOUNT_TAG_IDS.SELECTED }
        ]
      }),
      isEip1559Changed: mutation<IsEip1559ChangedMutationArg, IsEip1559Changed>(
        {
          queryFn: async (arg, _, __, baseQuery) => {
            // invalidate base cache of networks
            baseQuery(undefined).cache.clearNetworksRegistry()

            const { chainId, isEip1559 } = arg
            return {
              data: { id: chainId, isEip1559 }
            }
          },
          invalidatesTags: ['Network']
        }
      ),
      refreshNetworkInfo: mutation<boolean, void>({
        queryFn: async (arg, api, extraOptions, baseQuery) => {
          // invalidate base cache of networks
          baseQuery(undefined).cache.clearNetworksRegistry()
          // invalidates tags
          return {
            data: true
          }
        },
        // refresh networks & selected network
        invalidatesTags: ['Network']
      }),
      //
      // Prices
      //
      getTokenSpotPrices: query<SpotPriceRegistry, GetTokenSpotPricesArg>({
        queryFn: async (
          { ids, timeframe },
          { dispatch },
          extraOptions,
          baseQuery
        ) => {
          try {
            const {
              data: { assetRatioService }
            } = baseQuery(undefined)

            const defaultFiatCurrency: string = await dispatch(
              walletApi.endpoints.getDefaultFiatCurrency.initiate(undefined)
            ).unwrap()

            // dedupe ids to prevent duplicate price requests
            const uniqueIds = [...new Set(ids)]

            const chunkedParams = []
            for (let i = 0; i < uniqueIds.length; i += maxBatchSizePrice) {
              chunkedParams.push(uniqueIds.slice(i, i + maxBatchSizePrice))
            }

            // Use maxConcurrentPriceRequests concurrent HTTP requests to
            // fetch prices, in batch of maxBatchSizePrice.
            const results = await mapLimit(
              chunkedParams,
              maxConcurrentPriceRequests,
              async function (params: string[]) {
                const { success, values } = await assetRatioService.getPrice(
                  params,
                  [defaultFiatCurrency],
                  timeframe ?? BraveWallet.AssetPriceTimeframe.Live
                )

                if (success && values) {
                  return values
                }

                console.log('Unable to fetch prices for batch:', params)
                const fallbackResults = await mapLimit(
                  params,
                  maxConcurrentPriceRequests,
                  async function (param: string) {
                    const { success, values } =
                      await assetRatioService.getPrice(
                        [param],
                        [defaultFiatCurrency],
                        timeframe ?? BraveWallet.AssetPriceTimeframe.Live
                      )

                    if (success) {
                      return values
                    }

                    return []
                  }
                )

                return fallbackResults.flat()
              }
            )

            return {
              data: results.flat().reduce((acc, assetPrice) => {
                acc[assetPrice.fromAsset.toLowerCase()] = assetPrice
                return acc
              }, {})
            }
          } catch (error) {
            const msg = `Unable to fetch prices`
            console.error(`${msg}: ${error}`)
            return {
              error: msg
            }
          }
        },
        providesTags: (result, error, { ids, timeframe }) =>
          ids.map((id) => ({
            type: 'TokenSpotPrices',
            id: `${id}-${timeframe ?? BraveWallet.AssetPriceTimeframe.Live}`
          }))
      }),
      //
      // Tokens
      //
      getTokensRegistry: query<BlockchainTokenEntityAdaptorState, void>({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          try {
            const {
              cache,
              data: { blockchainRegistry }
            } = baseQuery(undefined)

            const networksRegistry = await cache.getNetworksRegistry()
            const networksList: BraveWallet.NetworkInfo[] =
              getEntitiesListFromEntityState(
                networksRegistry,
                networksRegistry.visibleIds
              )

            const tokenIdsByChainId: Record<string, string[]> = {}
            const tokenIdsByCoinType: Record<BraveWallet.CoinType, string[]> =
              {}

            const getTokensList = async () => {
              const tokenListsForNetworks = await mapLimit(
                networksList,
                10,
                async (network: BraveWallet.NetworkInfo) => {
                  const networkId = networkEntityAdapter.selectId(network)

                  const { tokens } = await blockchainRegistry.getAllTokens(
                    network.chainId,
                    network.coin
                  )

                  const fullTokensListForChain: BraveWallet.BlockchainToken[] =
                    await mapLimit(
                      tokens,
                      10,
                      async (token: BraveWallet.BlockchainToken) => {
                        return addChainIdToToken(
                          await addLogoToToken(token),
                          network.chainId
                        )
                      }
                    )

                  tokenIdsByChainId[networkId] =
                    fullTokensListForChain.map(getAssetIdKey)

                  tokenIdsByCoinType[network.coin] = (
                    tokenIdsByCoinType[network.coin] || []
                  ).concat(tokenIdsByChainId[networkId] || [])

                  return fullTokensListForChain
                }
              )

              const flattenedTokensList = tokenListsForNetworks.flat(1)
              return flattenedTokensList
            }

            let flattenedTokensList = await getTokensList()

            // on startup, the tokens list returned from core may be empty
            const startDate = new Date()
            const timeoutSeconds = 5
            const timeoutMilliseconds = timeoutSeconds * 1000

            // retry until we have some tokens or the request takes too long
            while (
              // empty list
              flattenedTokensList.length < 1 &&
              // try until timeout reached
              new Date().getTime() - startDate.getTime() < timeoutMilliseconds
            ) {
              flattenedTokensList = await getTokensList()
            }

            // return an error on timeout, so a retry can be attempted
            if (flattenedTokensList.length === 0) {
              throw new Error('No tokens found in tokens registry')
            }

            const tokensByChainIdRegistry = blockchainTokenEntityAdaptor.setAll(
              {
                ...blockchainTokenEntityAdaptorInitialState,
                idsByChainId: tokenIdsByChainId,
                idsByCoinType: tokenIdsByCoinType,
                visibleTokenIds: [],
                visibleTokenIdsByChainId: {},
                visibleTokenIdsByCoinType: {}
              },
              flattenedTokensList
            )

            return {
              data: tokensByChainIdRegistry
            }
          } catch (error) {
            console.error(error)
            return {
              error: `Unable to fetch Tokens Registry: ${error}`
            }
          }
        },
        providesTags: cacher.providesRegistry('KnownBlockchainTokens'),
        onCacheEntryAdded: (_, { dispatch }) => {
          // re-parse transactions with new coins list
          dispatch(walletApi.endpoints.invalidateTransactionsCache.initiate())
        }
      }),
      getUserTokensRegistry: query<BlockchainTokenEntityAdaptorState, void>({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          try {
            return {
              data: await baseQuery(undefined).cache.getUserTokensRegistry()
            }
          } catch (error) {
            return {
              error: 'Unable to fetch UserTokens Registry'
            }
          }
        },
        onCacheEntryAdded: (_, { dispatch }) => {
          // re-parse transactions with new coins list
          dispatch(walletApi.endpoints.invalidateTransactionsCache.initiate())
        },
        providesTags: (res, err) =>
          err
            ? ['UNKNOWN_ERROR']
            : [{ type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY }]
      }),
      getERC721Metadata: query<
        {
          id: EntityId
          metadata?: ERC721Metadata
        },
        GetBlockchainTokenIdArg
      >({
        queryFn: async (tokenArg, api, extraOptions, baseQuery) => {
          if (!tokenArg.isErc721) {
            return {
              error: 'Cannot fetch erc-721 metadata for non erc-721 token'
            }
          }

          const { jsonRpcService } = baseQuery(undefined).data

          const result = await jsonRpcService.getERC721Metadata(
            tokenArg.contractAddress,
            tokenArg.tokenId,
            tokenArg.chainId
          )

          if (result.error || result.errorMessage) {
            return { error: result.errorMessage }
          }

          try {
            const metadata: ERC721Metadata = JSON.parse(result.response)
            return {
              data: {
                id: blockchainTokenEntityAdaptor.selectId(tokenArg),
                metadata
              }
            }
          } catch (error) {
            return {
              error: `error parsing erc721 metadata result: ${result.response}`
            }
          }
        },
        providesTags: cacher.cacheByBlockchainTokenArg('ERC721Metadata')
      }),
      addUserToken: mutation<{ id: EntityId }, BraveWallet.BlockchainToken>({
        queryFn: async (tokenArg, { dispatch }, extraOptions, baseQuery) => {
          const {
            cache,
            data: { braveWalletService }
          } = baseQuery(undefined)

          cache.clearUserTokensRegistry()

          if (tokenArg.isErc721) {
            // Get NFTMetadata
            const { metadata } = await dispatch(
              walletApi.endpoints.getERC721Metadata.initiate({
                coin: tokenArg.coin,
                chainId: tokenArg.chainId,
                contractAddress: tokenArg.contractAddress,
                isErc721: tokenArg.isErc721,
                tokenId: tokenArg.tokenId,
                isNft: tokenArg.isNft
              })
            ).unwrap()

            if (metadata?.image) {
              tokenArg.logo = metadata?.image || tokenArg.logo
            }
          }

          const result = await braveWalletService.addUserAsset(tokenArg)
          const tokenIdentifier =
            blockchainTokenEntityAdaptor.selectId(tokenArg)

          if (!result.success) {
            return {
              error: `Error adding user token: ${tokenIdentifier}`
            }
          }

          return {
            data: { id: tokenIdentifier }
          }
        },
        invalidatesTags: (_, __, tokenArg) => [
          { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
          { type: 'UserBlockchainTokens', id: getAssetIdKey(tokenArg) }
        ]
      }),
      removeUserToken: mutation<boolean, BraveWallet.BlockchainToken>({
        queryFn: async (tokenArg, api, extraOptions, baseQuery) => {
          const {
            cache,
            data: { braveWalletService }
          } = baseQuery(undefined)

          cache.clearUserTokensRegistry()

          try {
            const deleteResult = await braveWalletService.removeUserAsset(
              tokenArg
            )
            if (!deleteResult.success) {
              throw new Error()
            }
            return {
              data: true
            }
          } catch (error) {
            return {
              error: `Unable to remove user asset: ${getAssetIdKey(tokenArg)}`
            }
          }
        },
        invalidatesTags: (_, __, tokenArg) => [
          { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
          { type: 'UserBlockchainTokens', id: getAssetIdKey(tokenArg) }
        ]
      }),
      updateUserToken: mutation<{ id: EntityId }, BraveWallet.BlockchainToken>({
        queryFn: async (tokenArg, { dispatch }, extraOptions, baseQuery) => {
          const { cache } = baseQuery(undefined)
          cache.clearUserTokensRegistry()

          const deleted = await dispatch(
            walletApi.endpoints.removeUserToken.initiate(tokenArg)
          ).unwrap()
          if (deleted) {
            const result: { id: EntityId } = await dispatch(
              walletApi.endpoints.addUserToken.initiate(tokenArg)
            ).unwrap()
            return { data: { id: result.id } }
          }
          return {
            error: `unable to update token ${getAssetIdKey(tokenArg)}`
          }
        },
        invalidatesTags: (_, __, tokenArg) => [
          { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
          { type: 'UserBlockchainTokens', id: getAssetIdKey(tokenArg) }
        ],
        onQueryStarted: async (tokenArg, { queryFulfilled, dispatch }) => {
          const patchResult = dispatch(
            walletApi.util.updateQueryData(
              'getUserTokensRegistry',
              undefined,
              (draft: BlockchainTokenEntityAdaptorState) => {
                const tokenIdentifier =
                  blockchainTokenEntityAdaptor.selectId(tokenArg)
                draft.entities[tokenIdentifier] = tokenArg
              }
            )
          )
          try {
            await queryFulfilled
          } catch (error) {
            patchResult.undo()
          }
        }
      }),
      updateUserAssetVisible: mutation<boolean, SetUserAssetVisiblePayloadType>(
        {
          queryFn: async (
            { isVisible, token },
            api,
            extraOptions,
            baseQuery
          ) => {
            try {
              const {
                cache,
                data: { braveWalletService }
              } = baseQuery(undefined)

              cache.clearUserTokensRegistry()

              const { success } = await braveWalletService.setUserAssetVisible(
                token,
                isVisible
              )
              return { data: success }
            } catch (error) {
              return {
                error: `Could not user update asset visibility for token: ${
                  getAssetIdKey(token) // token identifier
                }`
              }
            }
          },
          invalidatesTags: (res, err, arg) =>
            res
              ? [
                  { type: 'UserBlockchainTokens', id: TOKEN_TAG_IDS.REGISTRY },
                  { type: 'UserBlockchainTokens', id: getAssetIdKey(arg.token) }
                ]
              : ['UNKNOWN_ERROR']
        }
      ),
      //
      // Token balances
      //
      getAccountTokenCurrentBalance: query<
        string,
        GetAccountTokenCurrentBalanceArg
      >({
        queryFn: async (
          { coin, address, token },
          { dispatch },
          extraOptions,
          baseQuery
        ) => {
          const { jsonRpcService } = baseQuery(undefined).data // apiProxy

          // Native asset balances
          if (isNativeAsset(token)) {
            // LOCALHOST
            if (
              token.chainId === BraveWallet.LOCALHOST_CHAIN_ID &&
              coin !== BraveWallet.CoinType.SOL
            ) {
              const { balance, error, errorMessage } =
                await jsonRpcService.getBalance(address, coin, token.chainId)

              // LOCALHOST will error until a local instance is detected
              // return a '0' balance until it's detected.
              if (error !== 0) {
                console.log(`getBalance error: ${errorMessage}`)
                return {
                  data: Amount.zero().format()
                }
              }

              return {
                data: Amount.normalize(balance)
              }
            }

            switch (coin) {
              case BraveWallet.CoinType.SOL: {
                const { balance, error } =
                  await jsonRpcService.getSolanaBalance(address, token.chainId)

                if (
                  token.chainId === BraveWallet.LOCALHOST_CHAIN_ID &&
                  error !== 0
                ) {
                  return { data: Amount.zero().format() }
                }

                return {
                  data: Amount.normalize(balance.toString())
                }
              }

              case BraveWallet.CoinType.FIL:
              case BraveWallet.CoinType.ETH:
              default: {
                const { balance, error, errorMessage } =
                  await jsonRpcService.getBalance(address, coin, token.chainId)

                if (error && errorMessage) {
                  return {
                    error: errorMessage
                  }
                }

                return {
                  data: Amount.normalize(balance)
                }
              }
            }
          }

          // Token Balances
          switch (coin) {
            // Ethereum Network tokens
            case BraveWallet.CoinType.ETH: {
              const { balance, error, errorMessage } = token.isErc721
                ? await jsonRpcService.getERC721TokenBalance(
                    token.contractAddress,
                    token.tokenId ?? '',
                    address,
                    token.chainId
                  )
                : await jsonRpcService.getERC20TokenBalance(
                    token.contractAddress,
                    address,
                    token.chainId
                  )

              if (error && errorMessage) {
                return { error: errorMessage }
              }

              return {
                data: Amount.normalize(balance)
              }
            }
            // Solana Network Tokens
            case BraveWallet.CoinType.SOL: {
              const { amount, uiAmountString, error, errorMessage } =
                await jsonRpcService.getSPLTokenAccountBalance(
                  address,
                  token.contractAddress,
                  token.chainId
                )

              if (error && errorMessage) {
                return { error: errorMessage }
              }

              return {
                data: token.isNft ? uiAmountString : amount
              }
            }

            // Other network type tokens
            default: {
              return {
                data: Amount.zero().format()
              }
            }
          }
        },
        providesTags: (result, error, { token }) =>
          cacher.cacheByBlockchainTokenArg('AccountTokenCurrentBalance')(
            result,
            error,
            token
          )
      }),
      getCombinedTokenBalanceForAllAccounts: query<
        string,
        GetCombinedTokenBalanceForAllAccountsArg
      >({
        queryFn: async (asset, { dispatch }, extraOptions, baseQuery) => {
          const { cache } = baseQuery(undefined)
          const { accounts } = await cache.getAllAccountsInfo()

          const accountsForAssetCoinType = accounts.filter(
            (account) => account.accountId.coin === asset.coin
          )

          const accountTokenBalancesForChainId: string[] = await mapLimit(
            accountsForAssetCoinType,
            10,
            async (account: BraveWallet.AccountInfo) => {
              const balance = await dispatch(
                walletApi.endpoints.getAccountTokenCurrentBalance.initiate({
                  coin: account.accountId.coin,
                  address: account.address,
                  token: {
                    chainId: asset.chainId,
                    coin: asset.coin,
                    contractAddress: asset.contractAddress,
                    isErc721: asset.isErc721,
                    isNft: asset.isNft,
                    tokenId: asset.tokenId
                  }
                })
              ).unwrap()

              return balance ?? ''
            }
          )

          // return a '0' balance until user has created a FIL or SOL account
          if (accountTokenBalancesForChainId.length === 0) {
            return {
              data: '0'
            }
          }

          const aggregatedAmount = accountTokenBalancesForChainId.reduce(
            function (totalBalance, itemBalance) {
              return itemBalance !== ''
                ? new Amount(totalBalance).plus(itemBalance).format()
                : itemBalance ?? '0'
            },
            '0'
          )

          return {
            data: aggregatedAmount
          }
        },
        providesTags: cacher.cacheByBlockchainTokenArg(
          'CombinedTokenBalanceForAllAccounts'
        )
      }),
      getTokenBalancesForChainId: query<
        TokenBalancesForChainId,
        GetTokenBalancesForChainIdArg
      >({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          // Construct arg to query native token for use in case the optimised
          // balance fetcher kicks in.
          const nativeTokenArg =
            arg.coin === CoinTypes.ETH
              ? arg.tokens.find(isNativeAsset)
              : arg.tokens // arg.coin is SOL
              ? arg.tokens.find(isNativeAsset)
              : {
                  coin: arg.coin,
                  chainId: arg.chainId,
                  contractAddress: '',
                  isErc721: false,
                  isNft: false,
                  tokenId: ''
                }

          const baseTokenBalances: TokenBalancesForChainId = {}
          if (nativeTokenArg) {
            const balance = await dispatch(
              walletApi.endpoints.getAccountTokenCurrentBalance.initiate(
                {
                  coin: arg.coin,
                  address:
                    arg.coin === CoinTypes.SOL ? arg.pubkey : arg.address,
                  token: nativeTokenArg
                },
                {
                  forceRefetch: true
                }
              )
            ).unwrap()
            baseTokenBalances[nativeTokenArg.contractAddress] = balance
          }

          try {
            const { jsonRpcService, braveWalletService } =
              baseQuery(undefined).data

            if (arg.coin === CoinTypes.ETH) {
              // jsonRpcService.getERC20TokenBalances cannot handle native
              // assets
              const contracts = arg.tokens
                .filter((token) => !isNativeAsset(token))
                .map((token) => token.contractAddress)
              if (contracts.length === 0) {
                return {
                  data: baseTokenBalances
                }
              }

              // TODO(josheleonard): aggresively cache this response since it
              // never changes
              const { chainIds: supportedChainIds } =
                await braveWalletService.getBalanceScannerSupportedChains()

              if (supportedChainIds.includes(arg.chainId)) {
                const result = await jsonRpcService.getERC20TokenBalances(
                  contracts,
                  arg.address,
                  arg.chainId
                )
                if (result.error === BraveWallet.ProviderError.kSuccess) {
                  return {
                    data: result.balances.reduce(
                      (acc, { balance, contractAddress }) => {
                        const token = arg.tokens.find(
                          (token) => token.contractAddress === contractAddress
                        )

                        const balanceAmount = balance
                          ? new Amount(balance)
                          : undefined

                        if (balanceAmount && token) {
                          return {
                            ...acc,
                            [contractAddress]: balanceAmount.format()
                          }
                        }

                        return acc
                      },
                      baseTokenBalances
                    )
                  }
                } else {
                  console.error(
                    `Error calling jsonRpcService.getERC20TokenBalances:
                    error=${result.errorMessage}
                    arg=`,
                    arg
                  )
                }
              }
            }

            if (arg.coin === CoinTypes.SOL && !arg.tokens) {
              const result = await jsonRpcService.getSPLTokenBalances(
                arg.pubkey,
                arg.chainId
              )

              if (result.error === BraveWallet.ProviderError.kSuccess) {
                return {
                  data: result.balances.reduce((acc, balanceResult) => {
                    if (balanceResult.amount) {
                      return {
                        ...acc,
                        [balanceResult.mint]: Amount.normalize(
                          balanceResult.amount
                        )
                      }
                    }

                    return acc
                  }, baseTokenBalances)
                }
              } else {
                console.error(
                  `Error calling jsonRpcService.getSPLTokenBalances:
                  error=${result.errorMessage}
                  arg=`,
                  arg
                )
              }
            }

            // Fallback to fetching individual balances
            const tokens = (arg.tokens ?? []).filter(
              (token) => !isNativeAsset(token)
            )

            const combinedBalancesResult = await mapLimit(
              tokens,
              10,
              async (token: BraveWallet.BlockchainToken) => {
                const result: string = await dispatch(
                  walletApi.endpoints.getAccountTokenCurrentBalance.initiate(
                    {
                      coin: arg.coin,
                      address:
                        arg.coin === CoinTypes.ETH ? arg.address : arg.pubkey,
                      token
                    },
                    {
                      forceRefetch: true
                    }
                  )
                ).unwrap()

                return {
                  key: token.contractAddress,
                  value: result
                }
              }
            )

            return {
              data: combinedBalancesResult
                .filter((item) => new Amount(item.value).gt(0))
                .reduce((obj, item) => {
                  obj[item.key] = item.value
                  return obj
                }, baseTokenBalances)
            }
          } catch (error) {
            console.error(error)
            return {
              error: `Unable to fetch getTokenBalancesForChainId(
                ${JSON.stringify(arg, undefined, 2)}
              )
              error: ${error?.message ?? error}`
            }
          }
        },
        providesTags: (balancesResult, err, arg) =>
          err
            ? ['TokenBalancesForChainId', 'UNKNOWN_ERROR']
            : (balancesResult &&
                Object.keys(balancesResult).map((tokenAddress) => ({
                  type: 'TokenBalancesForChainId',
                  id:
                    arg.coin === CoinTypes.ETH
                      ? `${arg.address}-${arg.coin}-${arg.chainId}-${tokenAddress}`
                      : `${arg.pubkey}-${arg.coin}-${arg.chainId}-${tokenAddress}`
                }))) || ['TokenBalancesForChainId']
      }),
      //
      // Transactions
      //
      getFVMAddress: query<GetFVMAddressResult, GetFVMAddressArg>({
        queryFn: async (arg, api, extraOptions, baseQuery) => {
          if (arg.coin !== BraveWallet.CoinType.FIL) {
            return { data: undefined }
          }
          try {
            const { braveWalletService } = baseQuery(undefined).data
            const convertResult = (await braveWalletService.convertFEVMToFVMAddress(arg.isMainNet, arg.addresses)).result
            return {
              data: convertResult
            }
          } catch(error) {
            return {
              error: `Unable to getFVMAddress`
            }
          }
        }
      }),
      invalidateTransactionsCache: mutation<boolean, void>({
        queryFn: () => {
          return { data: true }
        }, // no-op, uses invalidateTags
        invalidatesTags: ['Transactions']
      }),
      getTransactions: query<
        SerializableTransactionInfo[],
        GetTransactionsQueryArg
      >({
        queryFn: async (
          { address, coinType, chainId },
          { dispatch },
          extraOptions,
          baseQuery
        ) => {
          try {
            const {
              data: { txService },
              cache
            } = baseQuery(undefined)
            // TODO(apaymyshev): getAllTransactionInfo already supports getting
            // transaction for all accounts.
            const txInfos =
              address && coinType !== null
                ? (
                    await txService.getAllTransactionInfo(
                      coinType,
                      chainId,
                      address
                    )
                  ).transactionInfos
                : (
                    await mapLimit(
                      (
                        await cache.getAllAccountsInfo()
                      ).accounts,
                      10,
                      async (account: BraveWallet.AccountInfo) => {
                        const { transactionInfos } =
                          await txService.getAllTransactionInfo(
                            account.accountId.coin,
                            chainId,
                            account.address
                          )
                        return transactionInfos
                      }
                    )
                  ).flat(1)

            const nonRejectedTransactionInfos = txInfos
              // hide rejected txs
              .filter(
                (tx) => tx.txStatus !== BraveWallet.TransactionStatus.Rejected
              )
              .map(makeSerializableTransaction)

            const sortedTxs = sortTransactionByDate(
              nonRejectedTransactionInfos,
              'descending'
            )

            return {
              data: sortedTxs
            }
          } catch (error) {
            return {
              error: `Unable to fetch txs for address: ${address} (${coinType})
              error: ${error?.message ?? error}`
            }
          }
        },
        providesTags: (res, err, arg) =>
          err
            ? ['UNKNOWN_ERROR']
            : [
                ...TX_CACHE_TAGS.LISTS({
                  chainId: arg.chainId,
                  coin: arg.coinType,
                  fromAddress: arg.address
                }),
                ...TX_CACHE_TAGS.IDS((res || []).map(({ id }) => id))
              ]
      }),
      sendEthTransaction: mutation<
        { success: boolean },
        SendEthTransactionParams
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { braveWalletService, txService } = baseQuery(undefined).data
            /***
             * Determine whether to create a legacy or EIP-1559 transaction.
             *
             * isEIP1559 is true IFF:
             *   - network supports EIP-1559
             *
             *     AND
             *
             *   - keyring supports EIP-1559
             *     (ex: certain hardware wallets vendors)
             *
             *     AND
             *
             *   - payload: SendEthTransactionParams has NOT specified legacy
             *              gas-pricing fields.
             *
             * In all other cases, fallback to legacy gas-pricing fields.
             * For example, if network and keyring support EIP-1559, but the
             * legacy gasPrice field is specified in the payload, then type-0
             * transaction should be created.
             */
            const isEIP1559 =
              payload.gasPrice === undefined &&
              hasEIP1559Support(
                getAccountType(payload.fromAccount),
                payload.network
              )

            const { chainId } =
              await braveWalletService.getChainIdForActiveOrigin(
                BraveWallet.CoinType.ETH
              )

            const txData: BraveWallet.TxData = {
              nonce: '',
              // Estimated by eth_tx_service
              // if value is '' for legacy transactions
              gasPrice: isEIP1559 ? '' : payload.gasPrice || '',
              // Estimated by eth_tx_service if value is ''
              gasLimit: payload.gas || '',
              to: payload.to,
              value: payload.value,
              data: payload.data || [],
              signOnly: false,
              signedTransaction: ''
            }

            const txData1559: BraveWallet.TxData1559 = {
              baseData: txData,
              chainId,
              // Estimated by eth_tx_service if value is ''
              maxPriorityFeePerGas: payload.maxPriorityFeePerGas || '',
              // Estimated by eth_tx_service if value is ''
              maxFeePerGas: payload.maxFeePerGas || '',
              gasEstimation: undefined
            }

            const { errorMessage, success } = await txService.addUnapprovedTransaction(
              isEIP1559
                ? toTxDataUnion({ ethTxData1559: txData1559 })
                : toTxDataUnion({ ethTxData: txData }),
              payload.fromAccount.address,
              null,
              null
            )

            if (!success && errorMessage) {
              return {
                error: `Failed to create Eth transaction: ${
                  errorMessage || 'unknown error'
                } ::payload:: ${JSON.stringify(payload)}`
              }
            }

            return {
              data: { success }
            }
          } catch (error) {
            return {
              error: `Failed to create Eth transaction: ${
                error || 'unknown error'
              } ::payload:: ${JSON.stringify(payload)}`
            }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err
            ? []
            : TX_CACHE_TAGS.LISTS({
                coin: arg.fromAccount.accountId.coin,
                fromAddress: arg.fromAccount.accountId.address,
                chainId: null
              })
      }),
      sendFilTransaction: mutation<
        { success: boolean },
        SendFilTransactionParams
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { txService } = baseQuery(undefined).data

            const filTxData: BraveWallet.FilTxData = {
              nonce: payload.nonce || '',
              gasPremium: payload.gasPremium || '',
              gasFeeCap: payload.gasFeeCap || '',
              gasLimit: payload.gasLimit || '',
              maxFee: payload.maxFee || '0',
              to: payload.to,
              from: payload.fromAccount.address,
              value: payload.value
            }

            const { errorMessage, success } =
              await txService.addUnapprovedTransaction(
                toTxDataUnion({ filTxData: filTxData }),
                payload.fromAccount.address,
                null,
                null
              )

            if (!success && errorMessage) {
              return {
                error: `Failed to send Fil transaction: ${
                  errorMessage || 'unknown error'
                }`
              }
            }

            return {
              data: { success }
            }
          } catch (error) {
            return { error: 'Failed to send Fil transaction' }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err
            ? []
            : TX_CACHE_TAGS.LISTS({
                coin: arg.fromAccount.accountId.coin,
                fromAddress: arg.fromAccount.accountId.address,
                chainId: null
              })
      }),
      sendSolTransaction: mutation<
        { success: boolean },
        SendSolTransactionParams
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { solanaTxManagerProxy, txService } =
              baseQuery(undefined).data

            const {
              error,
              errorMessage: transferTxDataErrorMessage,
              txData
            } = await solanaTxManagerProxy.makeSystemProgramTransferTxData(
              payload.fromAccount.address,
              payload.to,
              BigInt(payload.value)
            )

            if (error && transferTxDataErrorMessage) {
              return {
                error: `Failed to make SOL system program transfer txData): ${
                  transferTxDataErrorMessage || 'unknown error'
                }`
              }
            }

            const { errorMessage, success } =
              await txService.addUnapprovedTransaction(
                toTxDataUnion({ solanaTxData: txData ?? undefined}),
                payload.fromAccount.address,
                null,
                null
              )

            if (!success && errorMessage) {
              return {
                error: `Failed to send Sol transaction: ${
                  errorMessage || 'unknown error'
                }`
              }
            }

            return {
              data: { success }
            }
          } catch (error) {
            return { error: 'Failed to send Sol transaction' }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err
            ? []
            : TX_CACHE_TAGS.LISTS({
                coin: arg.fromAccount.accountId.coin,
                fromAddress: arg.fromAccount.accountId.address,
                chainId: null
              })
      }),
      sendTransaction: mutation<
        { success: boolean },
        | SendEthTransactionParams
        | SendFilTransactionParams
        | SendSolTransactionParams
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const coin = payload.fromAccount.accountId.coin
            switch (coin) {
              case BraveWallet.CoinType.SOL: {
                const result: { success: boolean } = await dispatch(
                  walletApi.endpoints.sendSolTransaction.initiate(
                    payload as SendSolTransactionParams
                  )
                ).unwrap()
                return {
                  data: result
                }
              }
              case BraveWallet.CoinType.FIL: {
                const result: { success: boolean } = await dispatch(
                  walletApi.endpoints.sendFilTransaction.initiate(
                    payload as SendFilTransactionParams
                  )
                ).unwrap()
                return {
                  data: result
                }
              }
              case BraveWallet.CoinType.ETH: {
                const result: { success: boolean } = await dispatch(
                  walletApi.endpoints.sendEthTransaction.initiate(
                    payload as SendEthTransactionParams
                  )
                ).unwrap()
                return {
                  data: result
                }
              }
              default: {
                return {
                  error: `Unsupported coin type" ${coin}`
                }
              }
            }
          } catch (error) {
            console.log(
              'Sending unapproved transaction failed: ' +
                `from=${payload.fromAccount.address} err=${error}`
            )
            return {
              error: error
            }
          }
        }
        // invalidatesTags: handled by other 'send-X-Transaction` methods
      }),
      sendERC20Transfer: mutation<{ success: boolean }, ER20TransferParams>({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          const errMsg = `Failed making ERC20 transfer data, to: ${
            payload.to //
          }, value: ${
            payload.value //
          }`
          try {
            const { ethTxManagerProxy } = baseQuery(undefined).data
            const { data, success } =
              await ethTxManagerProxy.makeERC20TransferData(
                payload.to,
                payload.value
              )
            if (!success) {
              console.error(errMsg)
              return {
                error: errMsg
              }
            }

            const result: { success: boolean } = await dispatch(
              walletApi.endpoints.sendTransaction.initiate({
                network: payload.network,
                fromAccount: payload.fromAccount,
                to: payload.contractAddress,
                value: '0x0',
                gas: payload.gas,
                gasPrice: payload.gasPrice,
                maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
                maxFeePerGas: payload.maxFeePerGas,
                data
              })
            ).unwrap()

            return {
              data: result
            }
          } catch (error) {
            return { error: errMsg }
          }
        }
        // invalidatesTags: handled by other 'send-X-Transaction` methods
      }),
      sendSPLTransfer: mutation<{ success: boolean }, SPLTransferFromParams>({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { solanaTxManagerProxy, txService } =
              baseQuery(undefined).data

            const { errorMessage: transferTxDataErrorMessage, txData } =
              await solanaTxManagerProxy.makeTokenProgramTransferTxData(
                payload.network.chainId,
                payload.splTokenMintAddress,
                payload.fromAccount.address,
                payload.to,
                BigInt(payload.value)
              )

            if (!txData) {
              const errorMsg = `Failed making SPL transfer data
                to: ${payload.to}
                value: ${payload.value}
                error: ${transferTxDataErrorMessage}`
              console.error(errorMsg)
              return {
                error: errorMsg
              }
            }

            const { errorMessage, success } =
              await txService.addUnapprovedTransaction(
                toTxDataUnion({ solanaTxData: txData }),
                txData.feePayer,
                null,
                null
              )

            if (!success) {
              const errorMsg = `Unable to send SPL Transfer: ${errorMessage}`
              console.error(errorMessage)
              return {
                error: errorMsg
              }
            }

            return {
              data: {
                success
              }
            }
          } catch (error) {
            const msg = `SPL Transfer failed:
                to: ${payload.to}
                value: ${payload.value}`
            console.error(msg)
            return { error: msg }
          }
        },
        invalidatesTags: (res, err, arg) =>
          TX_CACHE_TAGS.LISTS({
            chainId: null,
            coin: arg.fromAccount.accountId.coin,
            fromAddress: arg.fromAccount.accountId.address
          })
      }),
      sendERC721TransferFrom: mutation<
        { success: boolean },
        ERC721TransferFromParams
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { ethTxManagerProxy } = baseQuery(undefined).data
            const { data, success } =
              await ethTxManagerProxy.makeERC721TransferFromData(
                payload.fromAccount.address,
                payload.to,
                payload.tokenId,
                payload.contractAddress
              )

            if (!success) {
              const msg = `Failed making ERC721 transferFrom data,
              from: ${payload.fromAccount.address}
              to: ${payload.to},
              tokenId: ${payload.tokenId}`
              console.log(msg)
              return { error: msg }
            }

            const result: { success: boolean } = await dispatch(
              walletApi.endpoints.sendTransaction.initiate({
                network: payload.network,
                fromAccount: payload.fromAccount,
                to: payload.contractAddress,
                value: '0x0',
                gas: payload.gas,
                gasPrice: payload.gasPrice,
                maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
                maxFeePerGas: payload.maxFeePerGas,
                data
              })
            ).unwrap()

            return {
              data: result
            }
          } catch (error) {
            return { error: '' }
          }
        },
        invalidatesTags: (res, err, arg) =>
          TX_CACHE_TAGS.LISTS({
            chainId: null,
            coin: arg.fromAccount.accountId.coin,
            fromAddress: arg.fromAccount.accountId.address
          })
      }),
      sendETHFilForwarderTransfer: mutation<
        { success: boolean },
        ETHFilForwarderTransferFromParams
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { ethTxManagerProxy } = baseQuery(undefined).data
            const { data, success } =
              await ethTxManagerProxy.makeFilForwarderTransferData(payload.to)

            if (!success) {
              const msg = `Failed making FilForwarder transferFrom data,
            from: ${payload.fromAccount.address}
            to: ${payload.to}`
              console.log(msg)
              return { error: msg }
            }

            const result: { success: boolean } = await dispatch(
              walletApi.endpoints.sendTransaction.initiate({
                network: payload.network,
                fromAccount: payload.fromAccount,
                to: payload.contractAddress,
                value: payload.value,
                gas: payload.gas,
                gasPrice: payload.gasPrice,
                maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
                maxFeePerGas: payload.maxFeePerGas,
                data
              })
            ).unwrap()

            return {
              data: result
            }
          } catch (error) {
            return { error: '' }
          }
        },
        invalidatesTags: (res, err, arg) =>
          TX_CACHE_TAGS.LISTS({
            chainId: null,
            coin: arg.fromAccount.accountId.coin,
            fromAddress: arg.fromAccount.address
          })
      }),

      approveERC20Allowance: mutation<{ success: boolean }, ApproveERC20Params>(
        {
          queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
            try {
              const { ethTxManagerProxy } = baseQuery(undefined).data

              const { data, success } =
                await ethTxManagerProxy.makeERC20ApproveData(
                  payload.spenderAddress,
                  payload.allowance
                )

              if (!success) {
                const msg = `Failed making ERC20 approve data
                contract: ${payload.contractAddress}
                spender: ${payload.spenderAddress}
                allowance: ${payload.allowance}`
                console.error(msg)
                return {
                  error: msg
                }
              }

              const result: { success: boolean } = await dispatch(
                walletApi.endpoints.sendTransaction.initiate({
                  network: payload.network,
                  fromAccount: payload.fromAccount,
                  to: payload.contractAddress,
                  value: '0x0',
                  data
                })
              ).unwrap()

              return { data: result }
            } catch (error) {
              return { error: '' }
            }
          },
          invalidatesTags: (res, err, arg) =>
            TX_CACHE_TAGS.LISTS({
              chainId: null,
              coin: BraveWallet.CoinType.ETH,
              fromAddress: arg.fromAccount.address
            })
        }
      ),
      transactionStatusChanged: mutation<
        {
          success: boolean
          txId: string
          status: BraveWallet.TransactionStatus
        },
        Pick<SerializableTransactionInfo, 'txStatus' | 'id' | 'chainId'> & {
          fromAddress: string
          coinType: BraveWallet.CoinType
        }
      >({
        queryFn: async (arg) => {
          // no-op
          // uses 'invalidateTags' to handle data refresh
          return {
            data: { success: true, txId: arg.id, status: arg.txStatus }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err
            ? ['UNKNOWN_ERROR']
            : [
                TX_CACHE_TAGS.ID(arg.id),
                ...(arg.txStatus === BraveWallet.TransactionStatus.Confirmed
                  ? ([
                      'UserBlockchainTokens', // refresh all user tokens
                      'AccountTokenCurrentBalance',
                      'TokenSpotPrices'
                    ] as const)
                  : [])
              ],
        onQueryStarted: async (arg, { dispatch, queryFulfilled }) => {
          const txQueryArgsToUpdate: GetTransactionsQueryArg[] = [
            {
              address: arg.fromAddress,
              coinType: arg.coinType,
              chainId: arg.chainId
            },
            {
              address: arg.fromAddress,
              coinType: arg.coinType,
              chainId: null
            },
            {
              address: null,
              coinType: arg.coinType,
              chainId: arg.chainId
            },
            {
              address: null,
              coinType: null,
              chainId: null
            }
          ]

          const patchActions = txQueryArgsToUpdate.map((argsToUpdate) =>
            walletApi.util.updateQueryData(
              'getTransactions',
              argsToUpdate,
              (draft) => {
                const foundTx = draft.find((tx) => tx.id === arg.id)
                if (foundTx) {
                  foundTx.txStatus = arg.txStatus
                }
              }
            )
          )

          const patchResults: PatchCollection[] = []
          // Note: batching not needed if we can upgrade to react 18+
          batch(() => {
            for (const action of patchActions) {
              const patch = dispatch(action)
              patchResults.push(patch)
            }
          })

          try {
            await queryFulfilled
          } catch (error) {
            patchResults.forEach((patchResult) => {
              patchResult.undo()
            })
          }
        }
      }),
      approveTransaction: mutation<
        { success: boolean },
        Pick<SerializableTransactionInfo, 'id' | 'chainId' | 'txType'> & {
          coinType: BraveWallet.CoinType
        }
      >({
        queryFn: async (txInfo, { dispatch }, extraOptions, baseQuery) => {
          try {
            const api = baseQuery(undefined).data
            const { txService, braveWalletP3A } = api
            const result: {
              status: boolean
              errorUnion: BraveWallet.ProviderErrorUnion
              errorMessage: string
            } = await txService.approveTransaction(
              txInfo.coinType,
              txInfo.chainId,
              txInfo.id
            )

            const error =
              result.errorUnion.providerError ??
              result.errorUnion.solanaProviderError

            if (error && error !== BraveWallet.ProviderError.kSuccess) {
              return {
                error: `${error}: ${result.errorMessage}`
              }
            }

            if (shouldReportTransactionP3A({ txInfo })) {
              braveWalletP3A.reportTransactionSent(txInfo.coinType, true)
            }

            return {
              data: { success: true }
            }
          } catch (error) {
            return {
              error: `Unable to approve transaction: ${error}`
            }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err ? ['UNKNOWN_ERROR'] : [TX_CACHE_TAGS.ID(arg.id)]
      }),
      approveHardwareTransaction: mutation<
        { success: boolean },
        Pick<
          SerializableTransactionInfo,
          'id' | 'txDataUnion' | 'txType' | 'fromAddress' | 'chainId'
        >
      >({
        queryFn: async (txInfo, store, extraOptions, baseQuery) => {
          try {
            const { data, cache } = baseQuery(undefined)
            const apiProxy = data

            const accountsRegistry = await cache.getAccountsRegistry()
            const foundAccount = accountsRegistry.entities[txInfo.fromAddress]

            if (!foundAccount?.hardware) {
              return {
                error:
                  'failed to approve hardware transaction - ' +
                  `account not found or is not hardware: ${txInfo.fromAddress}`
              }
            }

            const hardwareAccount: BraveWallet.HardwareInfo =
              foundAccount.hardware

            if (apiProxy instanceof WalletPanelApiProxy) {
              navigateToConnectHardwareWallet(apiProxy, store)
            }

            if (hardwareAccount.vendor === BraveWallet.LEDGER_HARDWARE_VENDOR) {
              let success, error, code
              switch (foundAccount.accountId.coin) {
                case BraveWallet.CoinType.ETH:
                  ;({ success, error, code } =
                    await signLedgerEthereumTransaction(
                      apiProxy,
                      hardwareAccount.path,
                      txInfo,
                      foundAccount.accountId.coin
                    ))
                  break
                case BraveWallet.CoinType.FIL:
                  ;({ success, error, code } =
                    await signLedgerFilecoinTransaction(
                      apiProxy,
                      txInfo,
                      foundAccount.accountId.coin
                    ))
                  break
                case BraveWallet.CoinType.SOL:
                  ;({ success, error, code } =
                    await signLedgerSolanaTransaction(
                      apiProxy,
                      hardwareAccount.path,
                      txInfo,
                      foundAccount.accountId.coin
                    ))
                  break
                default:
                  await store.dispatch(PanelActions.navigateToMain())
                  return {
                    error: `unsupported coin type for hardware approval`
                  }
              }
              if (success) {
                store.dispatch(PanelActions.setSelectedTransactionId(txInfo.id))
                store.dispatch(PanelActions.navigateTo('transactionDetails'))
                apiProxy.panelHandler?.setCloseOnDeactivate(true)
                return {
                  data: { success: true }
                }
              }

              if (code !== undefined) {
                if (code === 'unauthorized') {
                  store.dispatch(
                    PanelActions.setHardwareWalletInteractionError(code)
                  )
                  return {
                    error: code
                  }
                }

                const deviceError = dialogErrorFromLedgerErrorCode(code)
                if (deviceError === 'transactionRejected') {
                  await store.dispatch(
                    walletApi.endpoints.rejectTransaction.initiate({
                      chainId: txInfo.chainId,
                      coinType: getCoinFromTxDataUnion(txInfo.txDataUnion),
                      id: txInfo.id
                    })
                  )
                  store.dispatch(PanelActions.navigateToMain())
                  return {
                    data: { success: true }
                  }
                }

                store.dispatch(
                  PanelActions.setHardwareWalletInteractionError(deviceError)
                )
                return {
                  error: deviceError
                }
              }

              if (error) {
                // TODO: handle non-device errors
                console.log(error)
                store.dispatch(PanelActions.navigateToMain())

                return {
                  error:
                    typeof error === 'object'
                      ? JSON.stringify(error)
                      : error || 'unknown error'
                }
              }
            } else if (
              hardwareAccount.vendor === BraveWallet.TREZOR_HARDWARE_VENDOR
            ) {
              const { success, error, deviceError } =
                await signTrezorTransaction(
                  apiProxy,
                  hardwareAccount.path,
                  txInfo
                )
              if (success) {
                store.dispatch(PanelActions.setSelectedTransactionId(txInfo.id))
                store.dispatch(PanelActions.navigateTo('transactionDetails'))
                apiProxy.panelHandler?.setCloseOnDeactivate(true)
                // By default the focus is moved to the browser window
                // automatically when Trezor popup closed which triggers an
                // OnDeactivate event that would close the wallet panel because
                // of the above API call. However, there could be times that
                // the above call happens after OnDeactivate event, so the
                // wallet panel would stay open after Trezor popup closed. As a
                // workaround, we manually set the focus back to wallet panel
                // here so it would trigger another OnDeactivate event when
                // user clicks outside of the wallet panel.
                apiProxy.panelHandler?.focus()
                return {
                  data: { success: true }
                }
              }

              if (deviceError === 'deviceBusy') {
                // do nothing as the operation is already in progress
                return {
                  data: { success: true }
                }
              }

              console.log(error)
              store.dispatch(
                walletApi.endpoints.rejectTransaction.initiate({
                  chainId: txInfo.chainId,
                  coinType: getCoinFromTxDataUnion(txInfo.txDataUnion),
                  id: txInfo.id
                })
              )
              store.dispatch(PanelActions.navigateToMain())
              return {
                data: { success: false }
              }
            }

            store.dispatch(PanelActions.navigateToMain())

            return {
              data: { success: true }
            }
          } catch (error) {
            return {
              error: `Unable to approve hardware transaction: ${error}`
            }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err ? ['UNKNOWN_ERROR'] : [TX_CACHE_TAGS.ID(arg.id)]
      }),
      rejectTransaction: mutation<
        { success: boolean },
        Pick<BraveWallet.TransactionInfo, 'id' | 'chainId'> & {
          coinType: BraveWallet.CoinType
        }
      >({
        queryFn: async (tx, api, extraOptions, baseQuery) => {
          try {
            const { txService } = baseQuery(undefined).data
            await txService.rejectTransaction(tx.coinType, tx.chainId, tx.id)
            return {
              data: { success: true }
            }
          } catch (error) {
            return {
              error: `Unable to reject transaction: ${error}`
            }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err ? [] : [TX_CACHE_TAGS.ID(arg.id)]
      }),
      rejectAllTransactions: mutation<{ success: boolean }, void>({
        queryFn: async (_arg, { dispatch }) => {
          try {
            const pendingTxs: SerializableTransactionInfo[] = (
              await dispatch(
                walletApi.endpoints.getTransactions.initiate({
                  chainId: null,
                  address: null,
                  coinType: null
                })
              ).unwrap()
            ).filter(
              (tx) => tx.txStatus === BraveWallet.TransactionStatus.Unapproved
            )

            await mapLimit(
              pendingTxs,
              10,
              async (tx: SerializableTransactionInfo) => {
                const { success } = await dispatch(
                  walletApi.endpoints.rejectTransaction.initiate({
                    coinType: getCoinFromTxDataUnion(tx.txDataUnion),
                    chainId: tx.chainId,
                    id: tx.id
                  })
                ).unwrap()
                return success
              }
            )

            return {
              data: { success: true }
            }
          } catch (error) {
            return {
              error: `Unable to reject all transactions: ${error}`
            }
          }
        }
        // invalidatesTags handled by rejectTransaction
      }),
      updateUnapprovedTransactionGasFields: mutation<
        { success: boolean },
        UpdateUnapprovedTransactionGasFieldsType
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { ethTxManagerProxy } = baseQuery(undefined).data

            const isEIP1559 =
              payload.maxPriorityFeePerGas !== undefined &&
              payload.maxFeePerGas !== undefined

            if (isEIP1559) {
              const result = await ethTxManagerProxy //
                .setGasFeeAndLimitForUnapprovedTransaction(
                  payload.chainId,
                  payload.txMetaId,
                  payload.maxPriorityFeePerGas || '',
                  payload.maxFeePerGas || '',
                  payload.gasLimit
                )

              if (!result.success) {
                throw new Error(
                  'Failed to update unapproved transaction: ' +
                    `id=${payload.txMetaId} ` +
                    `maxPriorityFeePerGas=${payload.maxPriorityFeePerGas}` +
                    `maxFeePerGas=${payload.maxFeePerGas}` +
                    `gasLimit=${payload.gasLimit}`
                )
              }

              return {
                data: result
              }
            }

            if (!payload.gasPrice) {
              return {
                error: 'Gas price is required to update transaction gas fields'
              }
            }

            const result = await ethTxManagerProxy //
              .setGasPriceAndLimitForUnapprovedTransaction(
                payload.chainId,
                payload.txMetaId,
                payload.gasPrice,
                payload.gasLimit
              )

            if (!result.success) {
              throw new Error(
                'Failed to update unapproved transaction: ' +
                  `id=${payload.txMetaId} ` +
                  `gasPrice=${payload.gasPrice}` +
                  `gasLimit=${payload.gasLimit}`
              )
            }

            return {
              data: result
            }
          } catch (error) {
            console.error(error)
            return {
              error: `An error occurred while updating an transaction's gas: ${
                error //
              }`
            }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err ? [TX_CACHE_TAGS.TXS_LIST] : [TX_CACHE_TAGS.ID(arg.txMetaId)]
      }),
      updateUnapprovedTransactionSpendAllowance: mutation<
        { success: boolean },
        UpdateUnapprovedTransactionSpendAllowanceType
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          const errMsg =
            'Failed to update unapproved transaction: ' +
            `id=${payload.txMetaId} ` +
            `allowance=${payload.allowance}`
          try {
            const { ethTxManagerProxy } = baseQuery(undefined).data
            const { data, success } =
              await ethTxManagerProxy.makeERC20ApproveData(
                payload.spenderAddress,
                payload.allowance
              )

            if (!success) {
              return {
                error: `Failed making ERC20 approve data, spender: ${
                  payload.spenderAddress //
                }, allowance: ${
                  payload.allowance //
                }`
              }
            }

            const result =
              await ethTxManagerProxy.setDataForUnapprovedTransaction(
                payload.chainId,
                payload.txMetaId,
                data
              )

            if (!result.success) {
              return {
                error: errMsg
              }
            }

            return {
              data: result
            }
          } catch (error) {
            return {
              error: errMsg + ` - ${error}`
            }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err ? [] : [TX_CACHE_TAGS.ID(arg.txMetaId)]
      }),
      updateUnapprovedTransactionNonce: mutation<
        { success: boolean },
        UpdateUnapprovedTransactionNonceType
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          const errorMsg =
            'Failed to update unapproved transaction nonce: ' +
            `id=${payload.txMetaId} ` +
            `nonce=${payload.nonce}`
          try {
            const { ethTxManagerProxy } = baseQuery(undefined).data

            const result =
              await ethTxManagerProxy.setNonceForUnapprovedTransaction(
                payload.chainId,
                payload.txMetaId,
                payload.nonce
              )

            if (!result.success) {
              return { error: errorMsg }
            }

            return { data: result }
          } catch (error) {
            return { error: errorMsg + ` - ${error}` }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err ? [] : [TX_CACHE_TAGS.ID(arg.txMetaId)]
      }),
      retryTransaction: mutation<{ success: boolean }, RetryTransactionPayload>(
        {
          queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
            try {
              const { txService } = baseQuery(undefined).data

              const result = await txService.retryTransaction(
                payload.coinType,
                payload.chainId,
                payload.transactionId
              )

              if (!result.success) {
                return {
                  error:
                    'Retry transaction failed: ' +
                    `id=${payload.transactionId} ` +
                    `err=${result.errorMessage}`
                }
              }

              return { data: result }
            } catch (error) {
              return {
                error:
                  'Retry transaction failed: ' +
                  `id=${payload.transactionId} ` +
                  `err=${error}`
              }
            }
          },
          invalidatesTags: (res, err, arg) =>
            err ? [] : [TX_CACHE_TAGS.ID(arg.transactionId)]
        }
      ),
      cancelTransaction: mutation<
        { success: boolean },
        CancelTransactionPayload
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { txService } = baseQuery(undefined).data

            const result = await txService.speedupOrCancelTransaction(
              payload.coinType,
              payload.chainId,
              payload.transactionId,
              true
            )

            if (!result.success) {
              return {
                error:
                  'Cancel transaction failed: ' +
                  `id=${payload.transactionId} ` +
                  `err=${result.errorMessage}`
              }
            }

            return {
              data: { success: result.success }
            }
          } catch (error) {
            return {
              error:
                'Cancel transaction failed: ' +
                `id=${payload.transactionId} ` +
                `err=${error}`
            }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err ? [] : [TX_CACHE_TAGS.ID(arg.transactionId)]
      }),
      speedupTransaction: mutation<
        { success: boolean },
        SpeedupTransactionPayload
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { txService } = baseQuery(undefined).data

            const result = await txService.speedupOrCancelTransaction(
              payload.coinType,
              payload.chainId,
              payload.transactionId,
              false
            )

            if (!result.success) {
              return {
                error:
                  'Speedup transaction failed: ' +
                  `id=${payload.transactionId} ` +
                  `err=${result.errorMessage}`
              }
            }

            return {
              data: { success: result.success }
            }
          } catch (error) {
            return {
              error:
                'Speedup transaction failed: ' +
                `id=${payload.transactionId} ` +
                `err=${error}`
            }
          }
        },
        invalidatesTags: (res, err, arg) =>
          err ? [] : [TX_CACHE_TAGS.ID(arg.transactionId)]
      }),
      newUnapprovedTxAdded: mutation<
        { success: boolean; txId: string },
        SerializableTransactionInfo
      >({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          apiProxyFetcher().pageHandler?.showApprovePanelUI()
          return {
            data: {
              success: true,
              txId: arg.id
            }
          }
        },
        invalidatesTags: (res, err, arg) =>
          // invalidate pending txs
          res
            ? TX_CACHE_TAGS.LISTS({
                chainId: arg.chainId,
                coin: getCoinFromTxDataUnion(arg.txDataUnion),
                fromAddress: arg.fromAddress
              })
            : []
      }),
      unapprovedTxUpdated: mutation<
        { success: boolean },
        SerializableTransactionInfo
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          return { data: { success: true } } // no-op (invalidate pending txs)
        },
        invalidatesTags: (_, err, arg) =>
          err ? [] : [TX_CACHE_TAGS.ID(arg.id)]
      }),
      getAddressByteCode: query<
        string,
        { address: string; coin: number; chainId: string }
      >({
        queryFn: async (arg, api, extraOptions, baseQuery) => {
          try {
            const { jsonRpcService } = baseQuery(undefined).data
            const { bytecode, error, errorMessage } =
              await jsonRpcService.getCode(arg.address, arg.coin, arg.chainId)
            if (error !== 0 && errorMessage) {
              return {
                error: errorMessage
              }
            }
            return {
              data: bytecode
            }
          } catch (error) {
            console.log(error)
            return {
              error: `Was unable to fetch bytecode for address: ${arg.address}.`
            }
          }
        }
      }),
      //
      // Transactions Fees
      //
      getGasEstimation1559: query<
        BraveWallet.GasEstimation1559,
        string // chainId
      >({
        queryFn: async (chainIdArg, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { data: api } = baseQuery(undefined)
            const { ethTxManagerProxy } = api

            const { estimation } = await ethTxManagerProxy.getGasEstimation1559(
              chainIdArg
            )

            if (estimation === null) {
              const msg = `Failed to fetch gas estimates for chainId: ${
                chainIdArg //
              }`
              console.warn(msg)
              return {
                error: msg
              }
            }

            return {
              data: estimation
            }
          } catch (error) {
            return {
              error: `Failed to estimate EVM gas: ${error}`
            }
          }
        },
        providesTags: ['GasEstimation1559']
      }),
      getSolanaEstimatedFee: query<string, { chainId: string; txId: string }>({
        queryFn: async (arg, api, extraOptions, baseQuery) => {
          try {
            const { solanaTxManagerProxy } = baseQuery(undefined).data
            const { errorMessage, fee } =
              await solanaTxManagerProxy.getEstimatedTxFee(
                arg.chainId,
                arg.txId
              )

            if (!fee) {
              throw new Error(errorMessage)
            }

            return {
              data: fee.toString()
            }
          } catch (error) {
            const msg = `Unable to fetch Solana fees - txId: ${arg.txId}`
            console.error(msg)
            console.error(error)
            return {
              error: msg
            }
          }
        },
        providesTags: (res, er, arg) => [
          { type: 'SolanaEstimatedFees', id: arg.txId }
        ]
      }),
      getNftDiscoveryEnabledStatus: query<boolean, void>({
        queryFn: async (_arg, _api, _extraOptions, _baseQuery) => {
          try {
            const { braveWalletService } = apiProxyFetcher()
            const result = await braveWalletService.getNftDiscoveryEnabled()
            return {
              data: result.enabled
            }
          } catch (error) {
            return { error: 'Failed to fetch NFT auto-discovery status' }
          }
        },
        providesTags: ['NftDiscoveryEnabledStatus']
      }),
      setNftDiscoveryEnabled: mutation<
        boolean, // success
        boolean
      >({
        queryFn: async (arg, _api, _extraOptions, _baseQuery) => {
          try {
            const { braveWalletService } = apiProxyFetcher()
            await braveWalletService.setNftDiscoveryEnabled(arg)

            return {
              data: true
            }
          } catch (error) {
            return {
              error: 'Failed to set NFT auto-discovery status'
            }
          }
        },
        invalidatesTags: ['NftDiscoveryEnabledStatus']
      })
    }}
  })
    // panel endpoints
    .injectEndpoints({
      endpoints: ({ mutation, query }) => ({
        openPanelUI: mutation<boolean, void>({
          queryFn(arg, api, extraOptions, baseQuery) {
            const { panelHandler } = apiProxyFetcher()
            panelHandler?.showUI()
            return { data: true }
          }
        }),
        closePanelUI: mutation<boolean, void>({
          queryFn(arg, api, extraOptions, baseQuery) {
            const { panelHandler } = apiProxyFetcher()
            panelHandler?.closeUI()
            return { data: true }
          }
        })
      })
    })
    // brave rewards endpoints
    .injectEndpoints({ endpoints: braveRewardsApiEndpoints })
    // tx simulation
    .injectEndpoints({ endpoints: transactionSimulationEndpoints })
}

export type WalletApi = ReturnType<typeof createWalletApi>
export const walletApi: WalletApi = createWalletApi()

export const {
  middleware: walletApiMiddleware,
  reducer: walletApiReducer,
  reducerPath: walletApiReducerPath,
  // hooks
  useAddUserTokenMutation,
  useApproveERC20AllowanceMutation,
  useApproveHardwareTransactionMutation,
  useApproveTransactionMutation,
  useCancelTransactionMutation,
  useClosePanelUIMutation,
  useGetAccountInfosRegistryQuery,
  useGetAccountTokenCurrentBalanceQuery,
  useGetAddressByteCodeQuery,
  useGetCombinedTokenBalanceForAllAccountsQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetERC721MetadataQuery,
  useGetEVMTransactionSimulationQuery,
  useGetExternalRewardsWalletQuery,
  useGetFVMAddressQuery,
  useGetGasEstimation1559Query,
  useGetIsTxSimulationEnabledQuery,
  useGetNetworksRegistryQuery,
  useGetNftDiscoveryEnabledStatusQuery,
  useGetRewardsBalanceQuery,
  useGetRewardsEnabledQuery,
  useGetSelectedAccountAddressQuery,
  useGetSelectedChainQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetSolanaTransactionSimulationQuery,
  useGetSwapSupportedNetworkIdsQuery,
  useGetTokenBalancesForChainIdQuery,
  useGetTokenSpotPricesQuery,
  useGetTokensRegistryQuery,
  useGetTransactionsQuery,
  useGetUserTokensRegistryQuery,
  useInvalidateTransactionsCacheMutation,
  useIsEip1559ChangedMutation,
  useLazyGetAccountInfosRegistryQuery,
  useLazyGetAccountTokenCurrentBalanceQuery,
  useLazyGetAddressByteCodeQuery,
  useLazyGetCombinedTokenBalanceForAllAccountsQuery,
  useLazyGetDefaultFiatCurrencyQuery,
  useLazyGetERC721MetadataQuery,
  useLazyGetEVMTransactionSimulationQuery,
  useLazyGetExternalRewardsWalletQuery,
  useLazyGetGasEstimation1559Query,
  useLazyGetIsTxSimulationEnabledQuery,
  useLazyGetNetworksRegistryQuery,
  useLazyGetNftDiscoveryEnabledStatusQuery,
  useLazyGetRewardsBalanceQuery,
  useLazyGetRewardsEnabledQuery,
  useLazyGetSelectedAccountAddressQuery,
  useLazyGetSelectedChainQuery,
  useLazyGetSolanaEstimatedFeeQuery,
  useLazyGetSolanaTransactionSimulationQuery,
  useLazyGetSwapSupportedNetworkIdsQuery,
  useLazyGetTokenBalancesForChainIdQuery,
  useLazyGetTokenSpotPricesQuery,
  useLazyGetTokensRegistryQuery,
  useLazyGetTransactionsQuery,
  useLazyGetUserTokensRegistryQuery,
  useNewUnapprovedTxAddedMutation,
  useOpenPanelUIMutation,
  usePrefetch,
  useRefreshNetworkInfoMutation,
  useRejectAllTransactionsMutation,
  useRejectTransactionMutation,
  useRemoveUserTokenMutation,
  useRetryTransactionMutation,
  useSendERC20TransferMutation,
  useSendERC721TransferFromMutation,
  useSendEthTransactionMutation,
  useSendFilTransactionMutation,
  useSendSolTransactionMutation,
  useSendSPLTransferMutation,
  useSendTransactionMutation,
  useSetDefaultFiatCurrencyMutation,
  useSetNetworkMutation,
  useSetNftDiscoveryEnabledMutation,
  useSetSelectedAccountMutation,
  useSetSelectedCoinMutation,
  useSpeedupTransactionMutation,
  useTransactionStatusChangedMutation,
  useUnapprovedTxUpdatedMutation,
  useUpdateUnapprovedTransactionGasFieldsMutation,
  useUpdateUnapprovedTransactionNonceMutation,
  useUpdateUnapprovedTransactionSpendAllowanceMutation,
  useUpdateUserAssetVisibleMutation,
  useUpdateUserTokenMutation,
} = walletApi

// Derived Data Queries
const emptyIds: string[] = []

export const useGetSwapSupportedNetworksQuery = () => {
  // queries
  const registryQueryResult = useGetNetworksRegistryQuery()
  const { data: networksRegistry } = registryQueryResult
  const swapNetworksQueryResult = useGetSwapSupportedNetworkIdsQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        isLoading: res.isLoading,
        error: res.error,
        data: selectSwapSupportedNetworksFromQueryResult(
          registryQueryResult,
          res.data || emptyIds
        )
      }),
      skip: !networksRegistry
    }
  )

  return swapNetworksQueryResult
}

export const useGetMainnetsQuery = (
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetNetworksRegistryQuery(
    undefined,
    {
      selectFromResult: res => ({
        isLoading: res.isLoading,
        error: res.error,
        data: selectMainnetNetworksFromQueryResult(res),
      }),
      skip: opts?.skip
    }
  )

  return queryResults
}

export const useGetNetworksQuery = (
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetNetworksRegistryQuery(
    undefined,
    {
      selectFromResult: res => ({
        isLoading: res.isLoading,
        error: res.error,
        data: selectAllNetworksFromQueryResult(res),
      }),
      skip: opts?.skip
    }
  )

  return queryResults
}

export const useGetOffRampNetworksQuery = (
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetNetworksRegistryQuery(
    undefined,
    {
      selectFromResult: res => ({
        isLoading: res.isLoading,
        error: res.error,
        data: selectOffRampNetworksFromQueryResult(res),
      }),
      skip: opts?.skip
    }
  )

  return queryResults
}

export const useGetOnRampNetworksQuery = (
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetNetworksRegistryQuery(
    undefined,
    {
      selectFromResult: res => ({
        isLoading: res.isLoading,
        error: res.error,
        data: selectOnRampNetworksFromQueryResult(res),
      }),
      skip: opts?.skip
    }
  )

  return queryResults
}

export const useGetVisibleNetworksQuery = (
  arg?: undefined,
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetNetworksRegistryQuery(
    arg,
    {
      selectFromResult: res => ({
        isLoading: res.isLoading,
        error: res.error,
        data: selectVisibleNetworksFromQueryResult(res),
      }),
      skip: opts?.skip
    }
  )

  return queryResults
}

export const useGetNetworkQuery = (
  args:
    | {
        chainId: string
        coin: BraveWallet.CoinType
      }
    | typeof skipToken,
) => {
  return useGetNetworksRegistryQuery(
    args === skipToken ? skipToken : undefined,
    {
      selectFromResult: (res) => ({
        isLoading: res.isLoading,
        error: res.error,
        data:
          res.data && args !== skipToken
            ? res.data.entities[networkEntityAdapter.selectId(args)]
            : undefined
      }),
    }
  )
}

export const useSelectedCoinQuery = (
  arg?: undefined,
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetSelectedChainQuery(arg, {
    selectFromResult: (res) => ({ selectedCoin: res.data?.coin }),
    skip: opts?.skip
  })
  return queryResults
}

export type WalletApiSliceState = ReturnType<typeof walletApi['reducer']>
export type WalletApiSliceStateFromRoot = { walletApi: WalletApiSliceState }

export async function getSelectedNetwork(api: WalletApiProxy) {
  const { jsonRpcService, braveWalletService } = api

  const { coin: selectedCoin } = await braveWalletService.getSelectedCoin()

  if (selectedCoin === undefined) {
    throw new Error('selected coin was undefined')
  }

  const { originInfo } = await braveWalletService.getActiveOrigin()
  const { network } = await jsonRpcService.getNetwork(selectedCoin,
                                                      originInfo.origin)

  return network
}

//
// Internals
//
async function getEnabledCoinTypes(
  api: WalletApiProxy
) {
  const { walletHandler } = api

  // network type flags
  const {
    walletInfo: { isFilecoinEnabled, isSolanaEnabled }
  } = await walletHandler.getWalletInfo()

  // Get All Networks
  const enabledCoinTypes = SupportedCoinTypes.filter((coin) => {
    // MULTICHAIN: While we are still in development for FIL and SOL,
    // we will not use their networks unless enabled by brave://flags
    return (
      (coin === BraveWallet.CoinType.FIL && isFilecoinEnabled) ||
      (coin === BraveWallet.CoinType.SOL && isSolanaEnabled) ||
      coin === BraveWallet.CoinType.ETH
    )
  })

  return enabledCoinTypes
}

async function getAllNetworksList(
  api: WalletApiProxy
) {
  const { jsonRpcService } = api

  const enabledCoinTypes = await getEnabledCoinTypes(api)

  // Get All Networks
  const networks = (
    await mapLimit(
      enabledCoinTypes, 10, (async (coin: number) => {
        const { networks } = await jsonRpcService.getAllNetworks(coin)
        return networks
      })
    )
  ).flat(1)

  return networks
}

export async function getNetwork(
  api: WalletApiProxy,
  arg: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
): Promise<BraveWallet.NetworkInfo | undefined> {
  const networksList = await getAllNetworksList(api)

  return networksList.find(
    (n) => n.chainId === arg.chainId && n.coin === arg.coin
  )
}

export async function getVisibleNetworksList(
  api: WalletApiProxy
) {
  const { jsonRpcService } = api

  const enabledCoinTypes = await getEnabledCoinTypes(api)

  const networks = (
    await mapLimit(enabledCoinTypes, 10, async (coin: number) => {
      const { networks } = await jsonRpcService.getAllNetworks(coin)
      const { chainIds: hiddenChainIds } =
        await jsonRpcService.getHiddenNetworks(coin)
      return networks.filter((n) => !hiddenChainIds.includes(n.chainId))
    })
  ).flat(1)

  return networks
}

// panel internals
function navigateToConnectHardwareWallet(
  api: WalletPanelApiProxy,
  store: Pick<Store, 'dispatch' | 'getState'>
) {
  api.panelHandler.setCloseOnDeactivate(false)

  const selectedPanel: string = store.getState()?.panel?.selectedPanel

  if (selectedPanel === 'connectHardwareWallet') {
    return
  }

  store.dispatch(PanelActions.navigateTo('connectHardwareWallet'))
  store.dispatch(
    PanelActions.setHardwareWalletInteractionError(undefined)
  )
}
