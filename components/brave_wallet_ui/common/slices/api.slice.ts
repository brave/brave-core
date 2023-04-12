// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityId, ThunkDispatch } from '@reduxjs/toolkit'
import { createApi } from '@reduxjs/toolkit/query/react'

// types
import {
  ApproveERC20Params,
  BraveWallet,
  ER20TransferParams,
  ERC721Metadata,
  ERC721TransferFromParams,
  SendEthTransactionParams,
  SendFilTransactionParams,
  SendSolTransactionParams,
  SerializableTransactionInfo,
  SPLTransferFromParams,
  SupportedCoinTypes,
  SupportedOffRampNetworks,
  SupportedOnRampNetworks,
  SupportedTestNetworks,
  WalletInfoBase
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

// entities
import {
  networkEntityAdapter,
  emptyNetworksRegistry,
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
  accountInfoEntityAdaptorInitialState,
  AccountInfoEntity
} from './entities/account-info.entity'
import {
  blockchainTokenEntityAdaptor,
  blockchainTokenEntityAdaptorInitialState,
  BlockchainTokenEntityAdaptorState,
  combineTokenRegistries
} from './entities/blockchain-token.entity'
import { AccountTokenBalanceForChainId, TokenBalancesForChainId } from './entities/token-balance.entity'
import {
  TransactionEntity,
  transactionEntityAdapter,
  transactionEntityInitialState,
  TransactionEntityState
} from './entities/transaction.entity'

// utils
import { cacher } from '../../utils/query-cache-utils'
import getAPIProxy from '../async/bridge'
import type WalletApiProxy from '../wallet_api_proxy'
import {
  addChainIdToToken,
  getAssetIdKey,
  GetBlockchainTokenIdArg,
  isNativeAsset
} from '../../utils/asset-utils'
import { getEntitiesListFromEntityState } from '../../utils/entities.utils'
import { makeNetworkAsset } from '../../options/asset-options'
import { getTokenParam } from '../../utils/api-utils'
import { getAccountType, getAddressLabelFromRegistry } from '../../utils/account-utils'
import { getCoinFromTxDataUnion, getFilecoinKeyringIdFromNetwork, getNetworkFromTXDataUnion, hasEIP1559Support } from '../../utils/network-utils'
import Amount from '../../utils/amount'
import {
  accountHasInsufficientFundsForGas,
  accountHasInsufficientFundsForTransaction,
  findTransactionAccountFromRegistry,
  findTransactionToken,
  getETHSwapTransactionBuyAndSellTokens,
  getFormattedTransactionTransferredValue,
  getIsTxApprovalUnlimited,
  getTransactionApprovalTargetAddress,
  getTransactionDecimals,
  getTransactionErc721TokenId,
  getTransactionFormattedNativeCurrencyTotal,
  getTransactionFormattedSendCurrencyTotal,
  getTransactionIntent,
  getTransactionNonce,
  getTransactionToAddress,
  getTransactionTransferredToken,
  isFilecoinTransaction,
  isSendingToKnownTokenContractAddress,
  isSolanaSplTransaction,
  isSolanaTransaction,
  isSwapTransaction,
  ParsedTransactionWithoutFiatValues,
  parseTransactionFeesWithoutPrices,
  shouldReportTransactionP3A,
  transactionHasSameAddressError
} from '../../utils/tx-utils'
import { getLocale } from '../../../common/locale'
import { makeSerializableOriginInfo, makeSerializableTimeDelta } from '../../utils/model-serialization-utils'
import { SwapExchangeProxy } from '../constants/registry'
import {
  getTypedSolanaTxInstructions,
  TypedSolanaInstructionWithParams
} from '../../utils/solana-instruction-utils'
import { addLogoToToken } from '../async/lib'

export type AssetPriceById = BraveWallet.AssetPrice & {
  id: EntityId
  fromAssetId: EntityId
}

/**
 * A function to return the ref to either the main api proxy, or a mocked proxy
 * @returns function that returns an ApiProxy instance
 */
let apiProxyFetcher = () => getAPIProxy()

const emptyBalance = '0x0'

type GetAccountTokenCurrentBalanceArg = {
  account: Pick<AccountInfoEntity, 'address' | 'coin' | 'keyringId'>
  token: GetBlockchainTokenIdArg & Pick<BraveWallet.BlockchainToken, 'isNft'>
}

type GetCombinedTokenBalanceForAllAccountsArg =
  GetAccountTokenCurrentBalanceArg['token'] &
    Pick<BraveWallet.BlockchainToken, 'coin'>

type GetTokenBalancesForChainIdArg = {
  contracts: string[]
  address: string
  coin: BraveWallet.CoinType
  chainId: string
}

export interface IsEip1559ChangedMutationArg {
  id: string
  isEip1559: boolean
}

export interface GetAllTransactionsForAddressCoinTypeArg {
  address: string
  coinType: BraveWallet.CoinType
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

/** Non-redux-controlled state */
let selectedPendingTransactionId: string = ''

  /**
   * A place to store & manage dependency data for other queries
  */
  class BaseQueryCache {
    private _networksRegistry?: NetworksRegistry

    getWalletInfoBase = async () => {
      // TODO: cache this result when deprecating uncached accounts
      return (await apiProxyFetcher().walletHandler.getWalletInfo()).walletInfo
    }

    getNetworksRegistry = async () => {
      if (!this._networksRegistry) {
        const { jsonRpcService } = apiProxyFetcher()

        // network type flags
        const { isFilecoinEnabled, isSolanaEnabled } =
          await this.getWalletInfoBase()

        // Get all networks
        const filteredSupportedCoinTypes = SupportedCoinTypes.filter((coin) => {
          // FIL and SOL networks, unless enabled by brave://flags
          return (
            (coin === BraveWallet.CoinType.FIL && isFilecoinEnabled) ||
            (coin === BraveWallet.CoinType.SOL && isSolanaEnabled) ||
            coin === BraveWallet.CoinType.ETH
          )
        })

        const visibleIds: string[] = []
        const hiddenIds: string[] = []
        const idsByCoinType: Record<EntityId, EntityId[]> = {}
        const hiddenIdsByCoinType: Record<EntityId, string[]> = {}
        const mainnetIds: string[] = []
        const onRampIds: string[] = []
        const offRampIds: string[] = []

        // Get all networks for supported coin types
        const networkLists: BraveWallet.NetworkInfo[][] = await Promise.all(
          filteredSupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
            const { networks } = await jsonRpcService.getAllNetworks(coin)

            // hidden networks for coin
            let hiddenNetworkIds: string[] = []
            try {
              const { chainIds } = await jsonRpcService.getHiddenNetworks(coin)
              hiddenNetworkIds = chainIds.map(
                (id) =>
                  networkEntityAdapter.selectId({
                    coin,
                    chainId: id
                  }) as string
              )
            } catch (error) {
              console.log(error)
              console.log(
                `Unable to fetch Hidden ChainIds for coin: ${
                  coin //
                }`
              )
              throw new Error(
                `Unable to fetch Hidden ChainIds for coin: ${
                  coin //
                }`
              )
            }

            idsByCoinType[coin] = []
            hiddenIdsByCoinType[coin] = []

            networks.forEach(({ chainId, coin }) => {
              const networkId = networkEntityAdapter
                .selectId({
                  chainId,
                  coin
                })
                .toString()

              if (!SupportedTestNetworks.includes(chainId)) {
                // skip testnet & localhost chains
                mainnetIds.push(networkId)
              }

              if (hiddenNetworkIds.includes(networkId)) {
                hiddenIdsByCoinType[coin].push(networkId)
                hiddenIds.push(networkId)
              } else {
                // visible networks for coin
                idsByCoinType[coin].push(networkId)
                visibleIds.push(networkId)
              }

              // on-ramps
              if (SupportedOnRampNetworks.includes(chainId)) {
                onRampIds.push(networkId)
              }

              // off-ramps
              if (SupportedOffRampNetworks.includes(chainId)) {
                offRampIds.push(networkId)
              }
            })

            // all networks
            return networks
          })
        )

        const networksList = networkLists.flat(1)

        // normalize list into a registry
        const normalizedNetworksState = networkEntityAdapter.setAll(
          {
            ...emptyNetworksRegistry,
            idsByCoinType,
            hiddenIds,
            hiddenIdsByCoinType,
            visibleIds,
            onRampIds,
            offRampIds,
            mainnetIds
          },
          networksList
        )

        return normalizedNetworksState
      }
      return this._networksRegistry
    }

    clearNetworksRegistry = () => {
      this._networksRegistry = undefined
    }
  }

export function createWalletApi (
  getProxy: () => WalletApiProxy = () => getAPIProxy()
) {
  apiProxyFetcher = getProxy // update the proxy whenever a new api is created

  const baseQueryCache = new BaseQueryCache();

  const walletApi = createApi({
    reducerPath: 'walletApi',
    baseQuery: () => {
      return { data: apiProxyFetcher(), cache: baseQueryCache }
    },
    tagTypes: [
      ...cacher.defaultTags,
      'AccountInfos',
      'AccountTokenCurrentBalance',
      'CombinedTokenBalanceForAllAccounts',
      'TokenBalancesForChainId',
      'DefaultAccountAddresses',
      'DefaultFiatCurrency',
      'ERC721Metadata',
      'SolanaEstimatedFees',
      'GasEstimation1559',
      'KnownBlockchainTokens',
      'Network',
      'PendingTransactions',
      'SelectedAccountAddress',
      'TokenSpotPrice',
      'TransactionsForAccount',
      'UserBlockchainTokens',
      'WalletInfo'
    ],
    endpoints: ({ mutation, query }) => ({
      //
      // Account Info
      //
      getAccountInfosRegistry: query<AccountInfoEntityState, void>({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          const { cache } = baseQuery(undefined)
          const walletInfo: WalletInfoBase = await cache.getWalletInfoBase()
          const accountInfos = walletInfo.accountInfos.map<AccountInfoEntity>(
            (info) => {
              return {
                ...info,
                accountType: getAccountType(info),
                deviceId: info.hardware ? info.hardware.deviceId : ''
              }
            }
          )
          return {
            data: accountInfoEntityAdaptor.setAll(
              accountInfoEntityAdaptorInitialState,
              accountInfos
            )
          }
        },
        providesTags: cacher.providesRegistry('AccountInfos')
      }),
      getDefaultAccountAddresses: query<string[], void>({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          // apiProxy
          const { keyringService, jsonRpcService } = baseQuery(undefined).data

          // Get default account addresses for each CoinType
          const defaultAccountAddresses = await Promise.all(
            SupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
              const { chainId } = await jsonRpcService.getChainId(coin)
              const defaultAccount =
                coin === BraveWallet.CoinType.FIL
                  ? await keyringService.getFilecoinSelectedAccount(chainId)
                  : await keyringService.getSelectedAccount(coin)
              return defaultAccount.address
            })
          )

          // remove empty addresses
          const filteredDefaultAccountAddresses =
            defaultAccountAddresses.filter(
              (account: string | null): account is string =>
                account !== null && account !== ''
            )

          return {
            data: filteredDefaultAccountAddresses
          }
        },
        providesTags: ['DefaultAccountAddresses']
      }),
      setSelectedAccount: mutation<
        string,
        {
          address: string
          coin: BraveWallet.CoinType
        }
      >({
        queryFn: async ({ address, coin }, api, extraOptions, baseQuery) => {
          const { keyringService } = baseQuery(undefined).data // apiProxy
          await keyringService.setSelectedAccount(address, coin)
          return {
            data: address
          }
        },
        invalidatesTags: [
          'DefaultAccountAddresses',
          { type: 'Network', id: NETWORK_TAG_IDS.SELECTED },
          'SelectedAccountAddress'
        ]
      }),
      getSelectedAccountAddress: query<string, void>({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          const { keyringService, braveWalletService, jsonRpcService } =
            baseQuery(undefined).data // apiProxy

          const { coin: selectedCoin } =
            await braveWalletService.getSelectedCoin()

          let selectedAddress: string | null = null
          if (selectedCoin === BraveWallet.CoinType.FIL) {
            const { chainId } = await jsonRpcService.getChainId(selectedCoin)
            selectedAddress = (
              await keyringService.getFilecoinSelectedAccount(chainId)
            ).address
          } else {
            selectedAddress = (
              await keyringService.getSelectedAccount(selectedCoin)
            ).address
          }

          const accountsRegistry: AccountInfoEntityState = await dispatch(
            walletApi.endpoints.getAccountInfosRegistry.initiate(undefined)
          ).unwrap()
          const fallbackAccount = accountsRegistry[accountsRegistry.ids[0]]

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
        providesTags: ['SelectedAccountAddress']
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

            const chainIdsWithSupportFlags = await Promise.all(
              networksRegistry.ids.map(async (chainId) => {
                const { result } = await swapService.isSwapSupported(
                  chainId.toString()
                )
                return {
                  chainId,
                  supported: result
                }
              })
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
      getDefaultNetworks: query<BraveWallet.NetworkInfo[], void>({
        // We can probably remove this when all
        // Transactions and Sign-Message Requests include a chainId
        queryFn: async (arg, api, extraOptions, baseQuery) => {
          try {
            const { jsonRpcService } = baseQuery(undefined).data // apiProxy

            const defaultChains = await Promise.all(
              SupportedCoinTypes.map(async (coinType) => {
                const { network } = await jsonRpcService.getNetwork(coinType)
                return network
              })
            )

            return {
              data: defaultChains
            }
          } catch (error) {
            console.error(error)
            return {
              error: `Error occurred within "getDefaultNetworks": ${
                error.toString() //
              }`
            }
          }
        },
        providesTags: [{ type: 'Network', id: NETWORK_TAG_IDS.DEFAULTS }]
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
          'SelectedAccountAddress'
        ]
      }),
      setNetwork: mutation<
        Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>,
        Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
      >({
        queryFn: async (
          { chainId, coin },
          { dispatch },
          extraOptions,
          baseQuery
        ) => {
          try {
            const { jsonRpcService } = baseQuery(undefined).data

            await dispatch(
              walletApi.endpoints.setSelectedCoin.initiate(coin)
            ).unwrap()

            const { success } = await jsonRpcService.setNetwork(chainId, coin)
            if (!success) {
              throw new Error('jsonRpcService.setNetwork failed')
            }
            return {
              data: { chainId, coin }
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
          'DefaultAccountAddresses',
          'SelectedAccountAddress'
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
      getTokenSpotPrice: query<AssetPriceById, GetBlockchainTokenIdArg>({
        queryFn: async (tokenArg, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { assetRatioService } = baseQuery(undefined).data

            const defaultFiatCurrency = await dispatch(
              walletApi.endpoints.getDefaultFiatCurrency.initiate(undefined)
            ).unwrap()

            // send the correct token identifier to the ratio service
            const getPriceTokenParam = getTokenParam(tokenArg)

            // create a cache id using the provided args
            const tokenPriceCacheId = `${getPriceTokenParam}-${defaultFiatCurrency}`

            const { success, values } = await assetRatioService.getPrice(
              [getPriceTokenParam],
              [defaultFiatCurrency],
              0
            )

            if (!success || !values[0]) {
              throw new Error()
            }

            const tokenPrice: AssetPriceById = {
              id: tokenPriceCacheId,
              ...values[0],
              fromAsset: tokenArg.symbol.toLowerCase(),
              fromAssetId: getAssetIdKey(tokenArg)
            }

            return {
              data: tokenPrice
            }
          } catch (error) {
            return {
              error: `Unable to find price for token ${tokenArg.symbol}`
            }
          }
        },
        providesTags: cacher.cacheByIdResultProperty('TokenSpotPrice')
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
              const tokenListsForNetworks = await Promise.all(
                networksList.map(async (network) => {
                  const networkId = networkEntityAdapter.selectId(network)

                  const { tokens } = await blockchainRegistry.getAllTokens(
                    network.chainId,
                    network.coin
                  )

                  const fullTokensListForChain: BraveWallet.BlockchainToken[] =
                    await Promise.all(tokens.map(async (token) => {
                      return addChainIdToToken(
                        await addLogoToToken(token),
                        network.chainId
                      )
                    }))

                  tokenIdsByChainId[networkId] =
                    fullTokensListForChain.map(getAssetIdKey)

                  tokenIdsByCoinType[network.coin] = (
                    tokenIdsByCoinType[network.coin] || []
                  ).concat(tokenIdsByChainId[networkId] || [])

                  return fullTokensListForChain
                })
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
            const {
              cache,
              data: { braveWalletService }
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
            const visibleTokenIds: string[] = []
            const visibleTokenIdsByChainId: Record<string, string[]> = {}
            const visibleTokenIdsByCoinType: Record<
              BraveWallet.CoinType,
              string[]
            > = {}

            const userTokenListsForNetworks = await Promise.all(
              networksList.map(async (network) => {
                const networkId = networkEntityAdapter.selectId(network)

                const fullTokensListForNetwork: BraveWallet.BlockchainToken[] =
                  await fetchUserAssetsForNetwork(braveWalletService, network)

                tokenIdsByCoinType[network.coin] = (
                  tokenIdsByCoinType[network.coin] || []
                ).concat(tokenIdsByChainId[networkId] || [])

                tokenIdsByChainId[networkId] =
                  fullTokensListForNetwork.map(getAssetIdKey)

                const visibleTokensForNetwork: BraveWallet.BlockchainToken[] =
                  fullTokensListForNetwork.filter((t) => t.visible)

                visibleTokenIdsByChainId[networkId] =
                  visibleTokensForNetwork.map(getAssetIdKey)

                visibleTokenIdsByCoinType[network.coin] = (
                  visibleTokenIdsByCoinType[network.coin] || []
                ).concat(visibleTokenIdsByChainId[networkId] || [])

                visibleTokenIds.push(...visibleTokenIdsByChainId[networkId])

                return fullTokensListForNetwork
              })
            )

            const userTokensByChainIdRegistry =
              blockchainTokenEntityAdaptor.setAll(
                {
                  ...blockchainTokenEntityAdaptorInitialState,
                  idsByChainId: tokenIdsByChainId,
                  tokenIdsByChainId,
                  visibleTokenIds,
                  visibleTokenIdsByChainId,
                  visibleTokenIdsByCoinType,
                  idsByCoinType: tokenIdsByCoinType
                },
                userTokenListsForNetworks.flat(1)
              )

            return {
              data: userTokensByChainIdRegistry
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
        providesTags: cacher.providesRegistry('UserBlockchainTokens')
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
          const { braveWalletService } = baseQuery(undefined).data
          if (tokenArg.isErc721) {
            // Get NFTMetadata
            const { metadata } = await dispatch(
              walletApi.endpoints.getERC721Metadata.initiate({
                chainId: tokenArg.chainId,
                contractAddress: tokenArg.contractAddress,
                isErc721: tokenArg.isErc721,
                tokenId: tokenArg.tokenId,
                symbol: tokenArg.symbol
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
        invalidatesTags: cacher.invalidatesList('UserBlockchainTokens')
      }),
      removeUserToken: mutation<boolean, BraveWallet.BlockchainToken>({
        queryFn: async (tokenArg, api, extraOptions, baseQuery) => {
          const { braveWalletService } = baseQuery(undefined).data
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
        invalidatesTags: cacher.invalidatesList('UserBlockchainTokens')
      }),
      updateUserToken: mutation<{ id: EntityId }, BraveWallet.BlockchainToken>({
        queryFn: async (tokenArg, { dispatch }, extraOptions, baseQuery) => {
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
              const { braveWalletService } = baseQuery(undefined).data
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
          invalidatesTags: cacher.invalidatesList('UserBlockchainTokens')
        }
      ),
      //
      // Token balances
      //
      getAccountTokenCurrentBalance: query<
        AccountTokenBalanceForChainId,
        GetAccountTokenCurrentBalanceArg
      >({
        queryFn: async (
          { account, token },
          { dispatch },
          extraOptions,
          baseQuery
        ) => {
          const { jsonRpcService } = baseQuery(undefined).data // apiProxy

          // entity lookup ids
          const accountEntityId: EntityId = accountInfoEntityAdaptor.selectId({
            address: account.address
          })

          const networkChainId = token.chainId
          const networkCoinType = account.coin

          const tokenEntityId: EntityId =
            blockchainTokenEntityAdaptor.selectId(token)

          // create default response
          const emptyBalanceResult: AccountTokenBalanceForChainId = {
            accountEntityId,
            balance: emptyBalance,
            chainId: networkChainId,
            tokenEntityId
          }

          // Native asset balances
          if (isNativeAsset(token)) {
            const nativeAssetDefaultBalanceResult = emptyBalanceResult

            // LOCALHOST
            if (
              token.chainId === BraveWallet.LOCALHOST_CHAIN_ID &&
              networkCoinType !== BraveWallet.CoinType.SOL
            ) {
              const { balance, error, errorMessage } =
                await jsonRpcService.getBalance(
                  account.address,
                  networkCoinType,
                  networkChainId
                )

              // LOCALHOST will error until a local instance is detected
              // return a '0' balance until it's detected.
              if (error !== 0) {
                return {
                  error: errorMessage
                }
              }

              return {
                data: {
                  ...nativeAssetDefaultBalanceResult,
                  balance
                }
              }
            }

            switch (networkCoinType) {
              case BraveWallet.CoinType.SOL: {
                const { balance, error } =
                  await jsonRpcService.getSolanaBalance(
                    account.address,
                    networkChainId
                  )

                if (
                  networkChainId === BraveWallet.LOCALHOST_CHAIN_ID &&
                  error !== 0
                ) {
                  return { data: emptyBalanceResult }
                }

                return {
                  data: {
                    ...nativeAssetDefaultBalanceResult,
                    balance: balance.toString()
                  } as AccountTokenBalanceForChainId
                }
              }

              case BraveWallet.CoinType.FIL:
              case BraveWallet.CoinType.ETH:
              default: {
                if (BraveWallet.CoinType.FIL) {
                  // Get network keyring id
                  const filecoinKeyringIdFromNetwork =
                    getFilecoinKeyringIdFromNetwork({
                      chainId: token.chainId,
                      coin: account.coin
                    })

                  if (account.keyringId !== filecoinKeyringIdFromNetwork) {
                    return { data: emptyBalanceResult }
                  }
                }

                const { balance, error, errorMessage } =
                  await jsonRpcService.getBalance(
                    account.address,
                    networkCoinType,
                    token.chainId
                  )

                if (error && errorMessage) {
                  return {
                    error: errorMessage
                  }
                }

                return {
                  data: {
                    ...nativeAssetDefaultBalanceResult,
                    balance
                  } as AccountTokenBalanceForChainId
                }
              }
            }
          }

          // Token Balances
          const tokenDefaultBalanceResult = emptyBalanceResult

          switch (account.coin) {
            // Ethereum Network tokens
            case BraveWallet.CoinType.ETH: {
              const { balance, error, errorMessage } = token.isErc721
                ? await jsonRpcService.getERC721TokenBalance(
                    token.contractAddress,
                    token.tokenId ?? '',
                    account.address,
                    networkChainId
                  )
                : await jsonRpcService.getERC20TokenBalance(
                    token.contractAddress,
                    account.address,
                    token?.chainId ?? ''
                  )

              if (error && errorMessage) {
                return { error: errorMessage }
              }

              return {
                data: {
                  ...tokenDefaultBalanceResult,
                  balance
                } as AccountTokenBalanceForChainId
              }
            }
            // Solana Network Tokens
            case BraveWallet.CoinType.SOL: {
              const { amount, uiAmountString, error, errorMessage } =
                await jsonRpcService.getSPLTokenAccountBalance(
                  account.address,
                  token.contractAddress,
                  token.chainId
                )

              if (error && errorMessage) {
                return { error: errorMessage }
              }

              const accountTokenBalance: AccountTokenBalanceForChainId = {
                ...tokenDefaultBalanceResult,
                balance: token.isNft ? uiAmountString : amount
              }

              return {
                data: accountTokenBalance
              }
            }

            // Other network type tokens
            default: {
              return {
                data: emptyBalanceResult
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
          const accountsRegistry: AccountInfoEntityState = await dispatch(
            walletApi.endpoints.getAccountInfosRegistry.initiate(undefined)
          ).unwrap()
          const accounts = getEntitiesListFromEntityState(accountsRegistry)

          const accountsForAssetCoinType = accounts.filter(
            (account) => account.coin === asset.coin
          )

          const accountTokenBalancesForChainId: string[] = await Promise.all(
            accountsForAssetCoinType.map(async (account) => {
              const balanceResult: AccountTokenBalanceForChainId =
                await dispatch(
                  walletApi.endpoints.getAccountTokenCurrentBalance.initiate({
                    account: {
                      address: account.address,
                      coin: account.coin,
                      keyringId: account.keyringId
                    },
                    token: {
                      chainId: asset.chainId,
                      contractAddress: asset.contractAddress,
                      isErc721: asset.isErc721,
                      isNft: asset.isNft,
                      symbol: asset.symbol,
                      tokenId: asset.tokenId
                    }
                  })
                ).unwrap()

              return balanceResult?.balance ?? ''
            })
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
          try {
            const { jsonRpcService } = baseQuery(undefined).data

            if (arg.coin === BraveWallet.CoinType.ETH) {
              const result = await jsonRpcService.getERC20TokenBalances(
                arg.contracts,
                arg.address,
                arg.chainId
              )
              if (result.error === BraveWallet.ProviderError.kSuccess) {
                return {
                  data: result.balances.reduce((acc, balanceResult) => {
                    if (balanceResult.balance) {
                      return {
                        ...acc,
                        [balanceResult.contractAddress]: Amount.normalize(
                          balanceResult.balance
                        )
                      }
                    }

                    return acc
                  }, {})
                }
              } else {
                return {
                  error: result.errorMessage
                }
              }
            }

            return {
              error: `Unsupported CoinType: ${arg.coin}`
            }
          } catch (error) {
            console.error(error)
            return {
              error: `Unable to fetch pending transactions
              error: ${error?.message ?? error}`
            }
          }
        },
        providesTags: (res, err, arg) =>
          err
            ? ['UNKNOWN_ERROR']
            : [
                {
                  type: 'TokenBalancesForChainId',
                  id: `${arg.address}-${arg.coin}-${arg.chainId}`
                }
              ]
      }),
      //
      // Transactions
      //
      getAllPendingTransactions: query<TransactionEntity[], void>({
        queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
          try {
            const {
              data: { txService },
              cache
            } = baseQuery(undefined)

            // accounts
            const accountsRegistry: AccountInfoEntityState = await dispatch(
              walletApi.endpoints.getAccountInfosRegistry.initiate(undefined)
            ).unwrap()
            const accounts = getEntitiesListFromEntityState(accountsRegistry)

            // networks
            const networksRegistry = await cache.getNetworksRegistry()

            // user tokens
            const userTokensRegistry: BlockchainTokenEntityAdaptorState =
              await dispatch(
                walletApi.endpoints.getUserTokensRegistry.initiate(undefined)
              ).unwrap()

            // known tokens
            let tokensRegistry = blockchainTokenEntityAdaptorInitialState
            try {
              await dispatch(
                walletApi.endpoints.getTokensRegistry.initiate(undefined)
              ).unwrap()
            } catch (error) {}

            // combined token registry
            const combinedTokenRegistry = combineTokenRegistries(
              tokensRegistry,
              userTokensRegistry
            )

            // TODO: Core should allow fetching by chain Id instead of cointype
            const transactionInfoResults = await Promise.all(
              accounts.map(async (a) => {
                const { transactionInfos } =
                  await txService.getAllTransactionInfo(a.coin, a.address)

                // return only pending txs
                return transactionInfos.filter(
                  (tx: BraveWallet.TransactionInfo) =>
                    tx.txStatus === BraveWallet.TransactionStatus.Unapproved
                )
              })
            )

            const transactions = transactionInfoResults.flat()

            const parsedTransactionsWithoutPrices = await Promise.all(
              transactions.map(async (tx: BraveWallet.TransactionInfo) => {
                return await parseTransactionWithoutPricesAsync({
                  tx,
                  accountsRegistry,
                  networksRegistry,
                  tokensRegistry: combinedTokenRegistry,
                  dispatch
                })
              })
            )

            selectedPendingTransactionId =
              parsedTransactionsWithoutPrices[0]?.id || ''

            return {
              data: parsedTransactionsWithoutPrices
            }
          } catch (error) {
            console.error(error)
            return {
              error: `Unable to fetch pending transactions
              error: ${error?.message ?? error}`
            }
          }
        },
        providesTags: (res, err, arg) =>
          err ? ['UNKNOWN_ERROR'] : ['PendingTransactions']
      }),
      invalidateTransactionsCache: mutation<boolean, void>({
        queryFn: () => {
          return { data: true }
        }, // no-op, uses invalidateTags
        invalidatesTags: ['PendingTransactions', 'TransactionsForAccount']
      }),
      getAllTransactionsForAddressCoinType: query<
        TransactionEntityState,
        GetAllTransactionsForAddressCoinTypeArg
      >({
        queryFn: async (
          { address, coinType },
          { dispatch },
          extraOptions,
          baseQuery
        ) => {
          const isFil = coinType === BraveWallet.CoinType.FIL
          try {
            const {
              cache,
              data: { txService }
            } = baseQuery(undefined)

            // accounts
            const accountsRegistry: AccountInfoEntityState = await dispatch(
              walletApi.endpoints.getAccountInfosRegistry.initiate(undefined)
            ).unwrap()

            // networks
            const networksRegistry = await cache.getNetworksRegistry()

            // user tokens (skipped for FIL accounts)
            let userTokensRegistry: BlockchainTokenEntityAdaptorState =
              blockchainTokenEntityAdaptorInitialState
            try {
              userTokensRegistry = isFil
                ? await dispatch(
                    walletApi.endpoints.getUserTokensRegistry.initiate(
                      undefined
                    )
                  ).unwrap()
                : blockchainTokenEntityAdaptorInitialState
            } catch (error) {
              console.log(error)
            }

            // known tokens (skipped for FIL accounts)
            let tokensRegistry: BlockchainTokenEntityAdaptorState =
              blockchainTokenEntityAdaptorInitialState
            try {
              tokensRegistry = isFil
                ? await dispatch(
                    walletApi.endpoints.getTokensRegistry.initiate(undefined)
                  ).unwrap()
                : blockchainTokenEntityAdaptorInitialState
            } catch (error) {
              console.log(error)
            }

            // combined token registry
            const combinedTokenRegistry = combineTokenRegistries(
              tokensRegistry,
              userTokensRegistry
            )

            // TODO: Core should allow fetching by chain Id instead of cointype
            const { transactionInfos } = await txService.getAllTransactionInfo(
              coinType,
              address
            )

            const idsByChainId: Record<EntityId, EntityId[]> = {}
            const pendingIds: EntityId[] = []
            const pendingIdsByChainId: Record<EntityId, EntityId[]> = {}

            const parsedTransactionsWithoutPrices = await Promise.all(
              transactionInfos
                // hide rejected txs
                .filter(
                  (tx) => tx.txStatus !== BraveWallet.TransactionStatus.Rejected
                )
                .map(async (tx: BraveWallet.TransactionInfo) => {
                  const parsedTx = await parseTransactionWithoutPricesAsync({
                    tx,
                    accountsRegistry,
                    networksRegistry,
                    tokensRegistry: combinedTokenRegistry,
                    dispatch
                  })

                  const networkId = networkEntityAdapter.selectId({
                    chainId: parsedTx.chainId,
                    coin: parsedTx.coinType
                  })

                  // track txs by chain
                  if (idsByChainId[networkId]) {
                    idsByChainId[networkId].push(tx.id)
                  } else {
                    idsByChainId[networkId] = [tx.id]
                  }

                  // track pending txs
                  if (
                    tx.txStatus === BraveWallet.TransactionStatus.Unapproved
                  ) {
                    pendingIds.push(tx.id)
                    // track pending txs by chain
                    if (pendingIdsByChainId[networkId]) {
                      pendingIdsByChainId[networkId].push(tx.id)
                    } else {
                      pendingIdsByChainId[networkId] = [tx.id]
                    }
                  }

                  return parsedTx
                })
            )

            const state: TransactionEntityState =
              transactionEntityAdapter.setAll(
                {
                  ...transactionEntityInitialState,
                  idsByChainId,
                  pendingIds,
                  pendingIdsByChainId
                },
                parsedTransactionsWithoutPrices
              )

            return {
              data: state
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
                {
                  type: 'TransactionsForAccount',
                  id: `${arg.address}-${arg.coinType}`
                }
              ]
      }),
      sendEthTransaction: mutation<
        { success: boolean },
        SendEthTransactionParams
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { jsonRpcService, txService } = baseQuery(undefined).data
            /***
             * Determine whether to create a legacy or EIP-1559 transaction.
             *
             * isEIP1559 is true IFF:
             *   - network supports EIP-1559
             *   - keyring supports EIP-1559
             *     (ex: certain hardware wallets vendors)
             *   - payload: SendEthTransactionParams has specified EIP-1559
             *              gas-pricing fields.
             *
             * In all other cases, fallback to legacy gas-pricing fields.
             */
            const isEIP1559 =
              // Transaction payload has hardcoded EIP-1559 gas fields.
              (payload.maxPriorityFeePerGas !== undefined &&
                payload.maxFeePerGas !== undefined) ||
              // Transaction payload does not have hardcoded legacy gas fields.
              payload.gasPrice === undefined ||
              // Check if network and keyring support EIP-1559.
              payload.hasEIP1559Support

            const { chainId } = await jsonRpcService.getChainId(
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

            // google closure is ok with undefined for other fields
            // but mojom runtime is not
            const txDataUnion = { ethTxData: txData } as BraveWallet.TxDataUnion

            const txData1559: BraveWallet.TxData1559 = {
              baseData: txData,
              chainId,
              // Estimated by eth_tx_service if value is ''
              maxPriorityFeePerGas: payload.maxPriorityFeePerGas || '',
              // Estimated by eth_tx_service if value is ''
              maxFeePerGas: payload.maxFeePerGas || '',
              gasEstimation: undefined
            }

            const txDataUnion1559 = {
              ethTxData1559: txData1559
            } as BraveWallet.TxDataUnion

            const { errorMessage, success } =
              await txService.addUnapprovedTransaction(
                isEIP1559 ? txDataUnion1559 : txDataUnion,
                payload.from,
                null,
                null
              )

            if (!success && errorMessage) {
              return {
                error: `Failed to send Eth transaction: ${
                  errorMessage || 'unknown error'
                }`
              }
            }

            return {
              data: { success }
            }
          } catch (error) {
            return { error: 'Failed to send Eth transaction' }
          }
        },
        invalidatesTags: (res, err, arg) => [
          { type: 'TransactionsForAccount', id: `${arg.from}-${arg.coin}` }
        ]
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
              from: payload.from,
              value: payload.value
            }

            const { errorMessage, success } =
              await txService.addUnapprovedTransaction(
                // google closure is ok with undefined for other fields
                // but mojom runtime is not
                { filTxData: filTxData } as BraveWallet.TxDataUnion,
                payload.from,
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
        invalidatesTags: (res, err, arg) => [
          { type: 'TransactionsForAccount', id: `${arg.from}-${arg.coin}` }
        ]
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
              payload.from,
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
                { solanaTxData: txData } as BraveWallet.TxDataUnion,
                payload.from,
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
        invalidatesTags: (res, err, arg) => [
          { type: 'TransactionsForAccount', id: `${arg.from}-${arg.coin}` }
        ]
      }),
      sendTransaction: mutation<
        { success: boolean },
        | Omit<SendEthTransactionParams, 'hasEIP1559Support'>
        | SendFilTransactionParams
        | SendSolTransactionParams
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            switch (payload.coin) {
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
                const accountsRegistry: AccountInfoEntityState = await dispatch(
                  walletApi.endpoints.getAccountInfosRegistry.initiate()
                ).unwrap()
                const selectedAccountAddress: string = await dispatch(
                  walletApi.endpoints.getSelectedAccountAddress.initiate()
                ).unwrap()
                const selectedAccount =
                  accountsRegistry.entities[
                    accountInfoEntityAdaptor.selectId({
                      address: selectedAccountAddress
                    })
                  ]

                const selectedNetwork = await getSelectedNetwork(
                  baseQuery(undefined).data
                )

                const result: { success: boolean } = await dispatch(
                  walletApi.endpoints.sendEthTransaction.initiate({
                    ...payload,
                    hasEIP1559Support:
                      !!selectedNetwork &&
                      !!selectedAccount &&
                      hasEIP1559Support(
                        selectedAccount.accountType,
                        selectedNetwork
                      )
                  })
                ).unwrap()
                return {
                  data: result
                }
              }
              default: {
                return {
                  error: `Unsupported coin type" ${payload.coin}`
                }
              }
            }
          } catch (error) {
            console.log(
              'Sending unapproved transaction failed: ' +
                `from=${payload.from} err=${error}`
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
                from: payload.from,
                to: payload.contractAddress,
                value: '0x0',
                gas: payload.gas,
                gasPrice: payload.gasPrice,
                maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
                maxFeePerGas: payload.maxFeePerGas,
                data,
                coin: BraveWallet.CoinType.ETH
              })
            ).unwrap()

            return {
              data: result
            }
          } catch (error) {
            return { error: errMsg }
          }
        }
      }),
      sendSPLTransfer: mutation<{ success: boolean }, SPLTransferFromParams>({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const { solanaTxManagerProxy, txService } =
              baseQuery(undefined).data

            const { errorMessage: transferTxDataErrorMessage, txData } =
              await solanaTxManagerProxy.makeTokenProgramTransferTxData(
                payload.splTokenMintAddress,
                payload.from,
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
                {
                  solanaTxData: txData
                } as BraveWallet.TxDataUnion,
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
        invalidatesTags: (res, err, arg) => [
          { type: 'TransactionsForAccount', id: `${arg.from}-${arg.coin}` }
        ]
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
                payload.from,
                payload.to,
                payload.tokenId,
                payload.contractAddress
              )

            if (!success) {
              const msg = `Failed making ERC721 transferFrom data,
              from: ${payload.from}
              to: ${payload.to},
              tokenId: ${payload.tokenId}`
              console.log(msg)
              return { error: msg }
            }

            const result: { success: boolean } = await dispatch(
              walletApi.endpoints.sendTransaction.initiate({
                from: payload.from,
                to: payload.contractAddress,
                value: '0x0',
                gas: payload.gas,
                gasPrice: payload.gasPrice,
                maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
                maxFeePerGas: payload.maxFeePerGas,
                data,
                coin: BraveWallet.CoinType.ETH
              })
            ).unwrap()

            return {
              data: result
            }
          } catch (error) {
            return { error: '' }
          }
        }
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
                  from: payload.from,
                  to: payload.contractAddress,
                  value: '0x0',
                  data,
                  coin: BraveWallet.CoinType.ETH
                })
              ).unwrap()

              return { data: result }
            } catch (error) {
              return { error: '' }
            }
          }
        }
      ),
      transactionStatusChanged: mutation<
        undefined,
        Pick<SerializableTransactionInfo, 'txStatus' | 'id'> & {
          fromAddress: string
          coinType: BraveWallet.CoinType
        }
      >({
        queryFn: async () => {
          // no-op
          // uses 'invalidateTags' to handle data refresh
          return { data: undefined }
        },
        invalidatesTags: (res, err, arg) => {
          const txTags = [
            'PendingTransactions',
            {
              type: 'TransactionsForAccount',
              id: `${arg.fromAddress}-${arg.coinType}`
            }
          ] as const
          switch (arg.txStatus) {
            case BraveWallet.TransactionStatus.Confirmed:
              return [
                'UserBlockchainTokens',
                'AccountTokenCurrentBalance',
                'TokenSpotPrice',
                ...txTags
                // token historical prices?
              ]
            case BraveWallet.TransactionStatus.Error:
              return txTags
            default:
              return []
          }
        },
        onQueryStarted: async (arg, { dispatch, queryFulfilled }) => {
          const patchResult = dispatch(
            walletApi.util.updateQueryData(
              'getAllTransactionsForAddressCoinType',
              { address: arg.fromAddress, coinType: arg.coinType },
              (draft) => {
                if (draft.entities[arg.id]) {
                  draft.entities[arg.id]!.status = arg.txStatus
                }
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
      approveTransaction: mutation<
        { success: boolean },
        Pick<
          SerializableTransactionInfo,
          'id' | 'txDataUnion' | 'txType' | 'fromAddress'
        >
      >({
        queryFn: async (txInfo, { dispatch }, extraOptions, baseQuery) => {
          try {
            const api = baseQuery(undefined).data
            const { txService, braveWalletP3A } = api
            const coin = getCoinFromTxDataUnion(txInfo.txDataUnion)
            const result: {
              status: boolean
              errorUnion: BraveWallet.ProviderErrorUnion
              errorMessage: string
            } = await txService.approveTransaction(coin, txInfo.id)

            const error =
              result.errorUnion.providerError ??
              result.errorUnion.solanaProviderError

            if (error !== BraveWallet.ProviderError.kSuccess) {
              console.error(`${result.errorMessage}

              ${JSON.stringify({
                transaction: txInfo,
                providerError: {
                  code: error,
                  message: result.errorMessage
                }
              })}
              `)

              return {
                error: result.errorMessage
              }
            }

            const selectedNetwork = await getSelectedNetwork(api)

            if (
              selectedNetwork &&
              shouldReportTransactionP3A(txInfo, selectedNetwork, coin)
            ) {
              braveWalletP3A.reportTransactionSent(coin, true)
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
          err
            ? []
            : [
                {
                  type: 'TransactionsForAccount',
                  id: `${arg.fromAddress}-${getCoinFromTxDataUnion(
                    arg.txDataUnion
                  )}`
                },
                'PendingTransactions'
              ]
      }),
      rejectTransaction: mutation<
        { success: boolean },
        Pick<TransactionEntity, 'id' | 'coinType'>
      >({
        queryFn: async (tx, api, extraOptions, baseQuery) => {
          try {
            const { txService } = baseQuery(undefined).data
            await txService.rejectTransaction(tx.coinType, tx.id)
            return {
              data: { success: true }
            }
          } catch (error) {
            return {
              error: `Unable to reject transaction: ${error}`
            }
          }
        },
        invalidatesTags: (res, err) => (err ? [] : ['PendingTransactions'])
      }),
      rejectAllTransactions: mutation<{ success: boolean }, void>({
        queryFn: async (_arg, { dispatch }) => {
          try {
            const pendingTxs: TransactionEntity[] = await dispatch(
              walletApi.endpoints.getAllPendingTransactions.initiate()
            ).unwrap()

            await Promise.all(
              pendingTxs.map(async ({ coinType, id }) => {
                const { success } = await dispatch(
                  walletApi.endpoints.rejectTransaction.initiate({
                    coinType,
                    id
                  })
                ).unwrap()
                return success
              })
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

            const { setGasFeeAndLimitForUnapprovedTransaction } =
              ethTxManagerProxy

            if (isEIP1559) {
              const result = await setGasFeeAndLimitForUnapprovedTransaction(
                payload.txMetaId,
                payload.maxPriorityFeePerGas || '',
                payload.maxFeePerGas || '',
                payload.gasLimit
              )

              if (!result.success) {
                return {
                  error:
                    'Failed to update unapproved transaction: ' +
                    `id=${payload.txMetaId} ` +
                    `maxPriorityFeePerGas=${payload.maxPriorityFeePerGas}` +
                    `maxFeePerGas=${payload.maxFeePerGas}` +
                    `gasLimit=${payload.gasLimit}`
                }
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

            const { setGasPriceAndLimitForUnapprovedTransaction } =
              ethTxManagerProxy

            const result = await setGasPriceAndLimitForUnapprovedTransaction(
              payload.txMetaId,
              payload.gasPrice,
              payload.gasLimit
            )

            if (!result.success) {
              return {
                error:
                  'Failed to update unapproved transaction: ' +
                  `id=${payload.txMetaId} ` +
                  `gasPrice=${payload.gasPrice}` +
                  `gasLimit=${payload.gasLimit}`
              }
            }

            return {
              data: result
            }
          } catch (error) {
            return {
              error: `An error occurred while updating an transaction's gas: ${
                error //
              }`
            }
          }
        },
        invalidatesTags: ['PendingTransactions']
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
        }
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
        }
      }),
      retryTransaction: mutation<{ success: boolean }, RetryTransactionPayload>(
        {
          queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
            try {
              const { txService } = baseQuery(undefined).data

              const result = await txService.retryTransaction(
                payload.coinType,
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
          // Refresh the transaction history of the origin account.
          invalidatesTags: (_, err, arg) =>
            err
              ? []
              : [
                  {
                    id: `${arg.fromAddress}-${arg.coinType}`,
                    type: 'TransactionsForAccount'
                  }
                ]
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
        // Refresh the transaction history of the origin account.
        invalidatesTags: (_, err, arg) =>
          err
            ? []
            : [
                {
                  id: `${arg.fromAddress}-${arg.coinType}`,
                  type: 'TransactionsForAccount'
                }
              ]
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
        // Refresh the transaction history of the origin account.
        invalidatesTags: (_, err, arg) =>
          err
            ? []
            : [
                {
                  id: `${arg.fromAddress}-${arg.coinType}`,
                  type: 'TransactionsForAccount'
                }
              ]
      }),
      newUnapprovedTxAdded: mutation<
        { success: boolean },
        SerializableTransactionInfo
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          return { data: { success: true } } // no-op (invalidate pending txs)
        },
        // Refresh the transaction history of the origin account.
        invalidatesTags: ['PendingTransactions']
      }),
      unapprovedTxUpdated: mutation<
        { success: boolean },
        SerializableTransactionInfo
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          return { data: { success: true } } // no-op (invalidate pending txs)
        },
        // Refresh the transaction history of the origin account.
        invalidatesTags: ['PendingTransactions']
      }),
      getSelectedPendingTransactionId: query<string, void>({
        queryFn: async (arg, api, extraOptions, baseQuery) => {
          return {
            data: selectedPendingTransactionId
          }
        }
      }),
      queueNextTransaction: mutation<
        { success: boolean },
        SerializableTransactionInfo
      >({
        queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
          try {
            const pendingTransactions = await dispatch(
              walletApi.endpoints.getAllPendingTransactions.initiate()
            ).unwrap()

            const index =
              pendingTransactions.findIndex(
                (tx) => tx.id === selectedPendingTransactionId
              ) + 1

            selectedPendingTransactionId =
              (pendingTransactions.length === index
                ? pendingTransactions[0]?.id
                : pendingTransactions[index]?.id) || ''

            return { data: { success: true } }
          } catch (error) {
            return {
              error: `${error}`
            }
          }
        }
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
      getGasEstimation1559: query<BraveWallet.GasEstimation1559, void>({
        queryFn: async (_, { dispatch }, extraOptions, baseQuery) => {
          try {
            const api = baseQuery(undefined).data
            const { ethTxManagerProxy } = api
            const selectedAccount = await querySelectedAccount(dispatch)
            const selectedNetwork = await getSelectedNetwork(api)

            if (
              selectedNetwork &&
              selectedAccount &&
              !hasEIP1559Support(selectedAccount.accountType, selectedNetwork)
            ) {
              return {
                error:
                  'EIP-1559 gas market unsupported for ' +
                  'selected account or network'
              }
            }

            const { estimation } =
              await ethTxManagerProxy.getGasEstimation1559()

            if (!estimation) {
              const msg = 'Failed to fetch gas estimates'
              console.error(msg)
              return {
                error: msg
              }
            }

            return {
              data: estimation
            }
          } catch (error) {
            return { error: 'Failed to estimate EVM gas' }
          }
        },
        providesTags: ['GasEstimation1559']
      }),
      getSolanaEstimatedFee: query<string, string>({
        queryFn: async (txIdArg, api, extraOptions, baseQuery) => {
          try {
            const { solanaTxManagerProxy } = baseQuery(undefined).data
            const { errorMessage, fee } =
              await solanaTxManagerProxy.getEstimatedTxFee(txIdArg)

            if (!fee) {
              throw new Error(errorMessage)
            }

            return {
              data: fee.toString()
            }
          } catch (error) {
            const msg = `Unable to fetch Solana fees - txId: ${txIdArg}`
            console.error(msg)
            console.error(error)
            return {
              error: msg
            }
          }
        },
        providesTags: (res, er, txIdArg) => [
          { type: 'SolanaEstimatedFees', id: txIdArg }
        ]
      }),
      refreshGasEstimates: mutation<
        boolean, // success
        Pick<TransactionEntity, 'id' | 'isSolanaTransaction'>
      >({
        queryFn: async (txInfo, { dispatch }, extraOptions, baseQuery) => {
          try {
            if (txInfo.isSolanaTransaction) {
              await dispatch(
                walletApi.endpoints.getSolanaEstimatedFee.initiate(txInfo.id)
              )
              return {
                data: true
              }
            }

            await dispatch(
              walletApi.endpoints.getGasEstimation1559.initiate()
            ).unwrap()

            return {
              data: true
            }
          } catch (error) {
            const msg = 'Failed to refresh gas estimates'
            console.error(msg, error)
            return { error: msg }
          }
        },
        invalidatesTags: (res, err, tx) =>
          tx.isSolanaTransaction
            ? [{ type: 'SolanaEstimatedFees', id: tx.id }]
            : ['GasEstimation1559']
      })
    })
  })
  return walletApi
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
  useApproveTransactionMutation,
  useCancelTransactionMutation,
  useGetAccountInfosRegistryQuery,
  useGetAccountTokenCurrentBalanceQuery,
  useGetAddressByteCodeQuery,
  useGetAllPendingTransactionsQuery,
  useGetAllTransactionsForAddressCoinTypeQuery,
  useGetCombinedTokenBalanceForAllAccountsQuery,
  useGetDefaultAccountAddressesQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetDefaultNetworksQuery,
  useGetERC721MetadataQuery,
  useGetGasEstimation1559Query,
  useGetNetworksRegistryQuery,
  useGetSelectedAccountAddressQuery,
  useGetSelectedChainQuery,
  useGetSelectedPendingTransactionIdQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetSwapSupportedNetworkIdsQuery,
  useGetTokenBalancesForChainIdQuery,
  useGetTokenSpotPriceQuery,
  useGetTokensRegistryQuery,
  useGetUserTokensRegistryQuery,
  useInvalidateTransactionsCacheMutation,
  useIsEip1559ChangedMutation,
  useLazyGetAccountInfosRegistryQuery,
  useLazyGetAccountTokenCurrentBalanceQuery,
  useLazyGetAddressByteCodeQuery,
  useLazyGetAllPendingTransactionsQuery,
  useLazyGetAllTransactionsForAddressCoinTypeQuery,
  useLazyGetCombinedTokenBalanceForAllAccountsQuery,
  useLazyGetDefaultAccountAddressesQuery,
  useLazyGetDefaultFiatCurrencyQuery,
  useLazyGetDefaultNetworksQuery,
  useLazyGetERC721MetadataQuery,
  useLazyGetGasEstimation1559Query,
  useLazyGetNetworksRegistryQuery,
  useLazyGetSelectedAccountAddressQuery,
  useLazyGetSelectedChainQuery,
  useLazyGetSelectedPendingTransactionIdQuery,
  useLazyGetSolanaEstimatedFeeQuery,
  useLazyGetSwapSupportedNetworkIdsQuery,
  useLazyGetTokenBalancesForChainIdQuery,
  useLazyGetTokenSpotPriceQuery,
  useLazyGetTokensRegistryQuery,
  useLazyGetUserTokensRegistryQuery,
  useNewUnapprovedTxAddedMutation,
  usePrefetch,
  useQueueNextTransactionMutation,
  useRefreshGasEstimatesMutation,
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
    | undefined,
  opts?: { skip?: boolean }
) => {
  return useGetNetworksRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data:
        res.data && args !== undefined
          ? res.data.entities[networkEntityAdapter.selectId(args)]
          : undefined
    }),
    skip: opts?.skip
  })
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

  const { network } = await jsonRpcService.getNetwork(selectedCoin)

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
    await Promise.all(
      enabledCoinTypes.map(async (coin) => {
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
    await Promise.all(
      enabledCoinTypes.map(async (coin) => {
        const { networks } = await jsonRpcService.getAllNetworks(coin)
        const { chainIds: hiddenChainIds } =
          await jsonRpcService.getHiddenNetworks(coin)
        return networks.filter((n) => !hiddenChainIds.includes(n.chainId))
      })
    )
  ).flat(1)
  return networks
}

async function fetchUserAssetsForNetwork (
  braveWalletService: BraveWallet.BraveWalletServiceRemote,
  network: BraveWallet.NetworkInfo
) {
  // Get a list of user tokens for each coinType and network.
  const { tokens } = await braveWalletService.getUserAssets(
    network.chainId,
    network.coin
  )

  // Adds a logo and chainId to each token object
  const tokenList: BraveWallet.BlockchainToken[] =
    await Promise.all(tokens.map(async (token) => {
      const updatedToken = await addLogoToToken(token)
      return addChainIdToToken(updatedToken, network.chainId)
    }))

  if (tokenList.length === 0) {
    // Creates a network's Native Asset if nothing was returned
    const nativeAsset = makeNetworkAsset(network)
    nativeAsset.logo = network.iconUrls[0] ?? ''
    nativeAsset.visible = false
    return [nativeAsset]
  }

  return tokenList
}

const querySelectedAccount = async (dispatch: ThunkDispatch<any, any, any>) => {
  const accountsRegistry: AccountInfoEntityState = await dispatch(
    walletApi.endpoints.getAccountInfosRegistry.initiate()
  ).unwrap()
  const selectedAccountAddress: string = await dispatch(
    walletApi.endpoints.getSelectedAccountAddress.initiate()
  ).unwrap()
  const selectedAccount =
    accountsRegistry.entities[
      accountInfoEntityAdaptor.selectId({
        address: selectedAccountAddress
      })
    ]
  return selectedAccount
}

export const parseTransactionWithoutPricesAsync = async ({
  tx,
  accountsRegistry,
  networksRegistry,
  tokensRegistry,
  dispatch
}: {
  tx: BraveWallet.TransactionInfo
  accountsRegistry: AccountInfoEntityState
  networksRegistry: NetworksRegistry
  tokensRegistry: BlockchainTokenEntityAdaptorState
  dispatch: ThunkDispatch<any, any, any>
}): Promise<ParsedTransactionWithoutFiatValues> => {
  const networks = getEntitiesListFromEntityState(networksRegistry)
  const transactionNetwork = getNetworkFromTXDataUnion(
    tx.txDataUnion,
    networks
  )

  // Tokens lists
  const fullTokensListIds = transactionNetwork?.coin !== undefined
    ? tokensRegistry.idsByCoinType[transactionNetwork.coin]
    : tokensRegistry.ids

  const userVisibleListIds = transactionNetwork?.coin !== undefined
    ? tokensRegistry.visibleTokenIdsByCoinType[transactionNetwork.coin]
    : tokensRegistry.visibleTokenIds

  const fullTokenList = getEntitiesListFromEntityState(
    tokensRegistry,
    fullTokensListIds
  )

  const userVisibleTokensList = getEntitiesListFromEntityState(
    tokensRegistry,
    userVisibleListIds
  )

  // network fees
  let solFeeEstimates: string = '0'
  try {
    solFeeEstimates = isSolanaTransaction(tx)
      ? await dispatch(
          walletApi.endpoints.getSolanaEstimatedFee.initiate(tx.id)
        ).unwrap()
      : '0'
  } catch (error) {
    console.error(error)
  }

  const to = getTransactionToAddress(tx)
  const combinedTokensList = userVisibleTokensList.concat(fullTokenList)
  const token = findTransactionToken(tx, combinedTokensList)
  const nativeAsset = makeNetworkAsset(transactionNetwork)
  const account = findTransactionAccountFromRegistry(accountsRegistry, tx)

  const {
    buyToken,
    sellToken,
    buyAmount,
    sellAmount,
    sellAmountWei,
    buyAmountWei
  } = getETHSwapTransactionBuyAndSellTokens({
    nativeAsset,
    tokensList: combinedTokensList,
    tx
  })

  // balances
  const emptyBalance = { balance: '0' }
  const { balance: accountNativeBalance } =
    nativeAsset && account
      ? await dispatch(
          walletApi.endpoints.getAccountTokenCurrentBalance.initiate(
            {
              account: {
                address: account.address,
                coin: account.coin,
                keyringId: account.keyringId
              },
              token: {
                chainId: nativeAsset.chainId,
                contractAddress: nativeAsset.contractAddress,
                isErc721: nativeAsset.isErc721,
                isNft: nativeAsset.isNft,
                symbol: nativeAsset.symbol,
                tokenId: nativeAsset.tokenId
              }
            }
          )
        ).unwrap()
      : emptyBalance

  const { balance: accountTokenBalance } =
    token && account
      ? await dispatch(
          walletApi.endpoints.getAccountTokenCurrentBalance.initiate(
            {
              account: {
                address: account.address,
                coin: account.coin,
                keyringId: account.keyringId
              },
              token: {
                chainId: token.chainId,
                contractAddress: token.contractAddress,
                isErc721: token.isErc721,
                isNft: token.isNft,
                symbol: token.symbol,
                tokenId: token.tokenId
              }
            }
          )
        ).unwrap()
      : emptyBalance

  const { balance: sellTokenBalance } =
    sellToken && account
      ? await dispatch(
          walletApi.endpoints.getAccountTokenCurrentBalance.initiate(
            {
              account: {
                address: account.address,
                coin: account.coin,
                keyringId: account.keyringId
              },
              token: {
                chainId: sellToken.chainId,
                contractAddress: sellToken.contractAddress,
                isErc721: sellToken.isErc721,
                isNft: sellToken.isNft,
                symbol: sellToken.symbol,
                tokenId: sellToken.tokenId
              }
            }
          )
        ).unwrap()
      : emptyBalance

  const {
    normalizedTransferredValue,
    normalizedTransferredValueExact,
    weiTransferredValue
  } = getFormattedTransactionTransferredValue({
    tx,
    txNetwork: transactionNetwork,
    token,
    sellToken
  })

  const erc721BlockchainToken = [
    BraveWallet.TransactionType.ERC721TransferFrom,
    BraveWallet.TransactionType.ERC721SafeTransferFrom
  ].includes(tx.txType) ? token : undefined

  const approvalTarget = getTransactionApprovalTargetAddress(tx)

  const {
    gasFee,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    isMissingGasLimit,
    maxFeePerGas,
    maxPriorityFeePerGas,
    isEIP1559Transaction
  } = parseTransactionFeesWithoutPrices(tx, { fee: solFeeEstimates })

  const insufficientFundsError = accountHasInsufficientFundsForTransaction({
    accountNativeBalance,
    accountTokenBalance,
    gasFee,
    tx,
    sellAmountWei,
    sellTokenBalance
  })

  const erc721TokenId = getTransactionErc721TokenId(tx)

  const missingGasLimitError = isMissingGasLimit
    ? getLocale('braveWalletMissingGasLimitError')
    : undefined

  const approvalTargetLabel = getAddressLabelFromRegistry(
    approvalTarget,
    accountsRegistry
  )

  const coinType = getCoinFromTxDataUnion(tx.txDataUnion)
  const createdTime = makeSerializableTimeDelta(tx.createdTime)

  const contractAddressError = isSendingToKnownTokenContractAddress(
    tx,
    combinedTokensList
  )
    ? getLocale('braveWalletContractAddressError')
    : undefined

  const decimals = getTransactionDecimals({
    tx,
    network: transactionNetwork,
    sellToken,
    erc721Token: erc721BlockchainToken,
    token
  })

  const instructions: TypedSolanaInstructionWithParams[] =
    getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData)

  const sameAddressError = transactionHasSameAddressError(tx)
    ? getLocale('braveWalletSameAddressError')
    : undefined

  const txToken = getTransactionTransferredToken({
    tx,
    txNetwork: transactionNetwork,
    token,
    sellToken
  })

  const symbol = txToken?.symbol || ''

  const intent = getTransactionIntent({
    normalizedTransferredValue,
    tx,
    buyAmount,
    buyToken,
    erc721TokenId,
    sellAmount,
    sellToken,
    token,
    transactionNetwork
  })

  const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
    accountNativeBalance,
    gasFee
  })

  const isSendingToZeroXExchangeProxy =
    tx.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() ===
    SwapExchangeProxy

  const formattedNativeCurrencyTotal =
    getTransactionFormattedNativeCurrencyTotal({
      gasFee,
      normalizedTransferredValue,
      tx,
      sellAmountWei: sellAmountWei.value?.toString(),
      sellToken,
      token,
      transferredValueWei: weiTransferredValue,
      txNetwork: transactionNetwork
    })

  const formattedSendCurrencyTotal = getTransactionFormattedSendCurrencyTotal({
    normalizedTransferredValue,
    sellToken,
    token,
    txNetwork: transactionNetwork,
    tx
  })

  return {
    accountAddress: account?.address || '',
    approvalTarget,
    approvalTargetLabel,
    buyToken,
    chainId: transactionNetwork?.chainId || '',
    coinType,
    contractAddressError,
    createdTime,
    decimals,
    erc721BlockchainToken,
    erc721TokenId,
    gasFee,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    hash: tx.txHash,
    id: tx.id,
    instructions,
    insufficientFundsError,
    insufficientFundsForGasError,
    intent,
    isApprovalUnlimited: getIsTxApprovalUnlimited(tx),
    isEIP1559Transaction,
    isFilecoinTransaction: isFilecoinTransaction(tx),
    isSendingToZeroXExchangeProxy,
    isSolanaDappTransaction: isSolanaTransaction(tx),
    isSolanaSPLTransaction: isSolanaSplTransaction(tx),
    isSolanaTransaction: isSolanaTransaction(tx),
    isSwap: isSwapTransaction(tx),
    maxFeePerGas,
    maxPriorityFeePerGas,
    minBuyAmount: buyAmount,
    minBuyAmountWei: buyAmountWei,
    missingGasLimitError,
    nonce: getTransactionNonce(tx),
    originInfo: makeSerializableOriginInfo(tx.originInfo),
    recipient: to,
    recipientLabel: getAddressLabelFromRegistry(to, accountsRegistry),
    sameAddressError,
    sellAmount,
    sellAmountWei,
    sellToken,
    sender: tx.fromAddress,
    senderLabel: getAddressLabelFromRegistry(tx.fromAddress, accountsRegistry),
    status: tx.txStatus,
    symbol,
    token,
    txType: tx.txType,
    value: normalizedTransferredValue,
    valueExact: normalizedTransferredValueExact,
    weiTransferredValue,
    formattedNativeCurrencyTotal,
    formattedSendCurrencyTotal
  }
}
