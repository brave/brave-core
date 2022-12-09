// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityId } from '@reduxjs/toolkit'
import { createApi } from '@reduxjs/toolkit/query/react'

// types
import {
  BraveWallet,
  ERC721Metadata,
  SupportedCoinTypes,
  WalletInfoBase
} from '../../constants/types'
import {
  IsEip1559Changed,
  SetUserAssetVisiblePayloadType
} from '../constants/action_types'

// entity adaptors
import {
  networkEntityAdapter,
  networkEntityInitalState,
  NetworkEntityState
} from './entities/network.entity'
import {
  AccountInfoEntityState,
  accountInfoEntityAdaptor,
  accountInfoEntityAdaptorInitialState
} from './entities/account-info.entity'
import {
  blockchainTokenEntityAdaptor,
  blockchainTokenEntityAdaptorInitialState,
  BlockchainTokenEntityAdaptorState
} from './entities/blockchain-token.entity'

// utils
import { cacher } from '../../utils/query-cache-utils'
import getAPIProxy from '../async/bridge'
import WalletApiProxy from '../wallet_api_proxy'
import { addLogoToToken, getAssetIdKey, GetBlockchainTokenIdArg } from '../../utils/asset-utils'
import { getEntitiesListFromEntityState } from '../../utils/entities.utils'
import { makeNetworkAsset } from '../../options/asset-options'
import { getTokenParam } from '../../utils/api-utils'

export type AssetPriceById = BraveWallet.AssetPrice & {
  id: EntityId
  fromAssetId: EntityId
}

export function createWalletApi (
  getProxy: () => WalletApiProxy = () => getAPIProxy()
) {
  const walletApi = createApi({
    reducerPath: 'walletApi',
    baseQuery: () => {
      return { data: getProxy() }
    },
    tagTypes: [
      ...cacher.defaultTags,
      'AccountInfos',
      'ChainIdForCoinType',
      'DefaultAccountAddresses',
      'DefaultFiatCurrency',
      'ERC721Metadata',
      'KnownBlockchainTokens',
      'Network',
      'SelectedAccountAddress',
      'SelectedChainId',
      'SelectedCoin',
      'TokenSpotPrice',
      'UserBlockchainTokens',
      'WalletInfo'
    ],
    endpoints: ({ mutation, query }) => ({
      //
      // Accounts & Wallet Info
      //
      getWalletInfoBase: query<WalletInfoBase, void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          const { walletHandler } = baseQuery(undefined).data
          const walletInfo: WalletInfoBase = await walletHandler.getWalletInfo()
          return {
            data: walletInfo
          }
        },
        providesTags: ['WalletInfo']
      }),
      getAccountInfosRegistry: query<AccountInfoEntityState, void>({
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          const walletInfo: WalletInfoBase = await dispatch(
            walletApi.endpoints.getWalletInfoBase.initiate()
          ).unwrap()
          const accountInfos: BraveWallet.AccountInfo[] = walletInfo.accountInfos
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
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          const { keyringService } = baseQuery(undefined).data // apiProxy

          // Get default account addresses for each CoinType
          const defaultAccountAddresses = await Promise.all(SupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
            const chainId: string = await dispatch(walletApi.endpoints.getChainIdForCoin.initiate(coin)).unwrap()
            const defaultAccount = coin === BraveWallet.CoinType.FIL
              ? await keyringService.getFilecoinSelectedAccount(chainId)
              : await keyringService.getSelectedAccount(coin)
            return defaultAccount.address
          }))

          // remove empty addresses
          const filteredDefaultAccountAddresses = defaultAccountAddresses
            .filter((account: string | null): account is string => account !== null && account !== '')

          return {
            data: filteredDefaultAccountAddresses
          }
        },
        providesTags: ['DefaultAccountAddresses']
      }),
      setSelectedAccount: mutation<string, {
        address: string
        coin: BraveWallet.CoinType
      }>({
        async queryFn ({ address, coin }, api, extraOptions, baseQuery) {
          const { keyringService } = baseQuery(undefined).data // apiProxy
          await keyringService.setSelectedAccount(address, coin)
          return {
            data: address
          }
        },
        invalidatesTags: ['SelectedAccountAddress']
      }),
      getSelectedAccountAddress: query<string, void>({
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          const { keyringService } = baseQuery(undefined).data // apiProxy

          const selectedCoin: number = await dispatch(walletApi.endpoints.getSelectedCoin.initiate()).unwrap()

          let selectedAddress: string | null = null
          if (selectedCoin === BraveWallet.CoinType.FIL) {
            const chainId: string = await dispatch(walletApi.endpoints.getChainIdForCoin.initiate(selectedCoin)).unwrap()
            selectedAddress = (await keyringService.getFilecoinSelectedAccount(chainId)).address
          } else {
            selectedAddress = (await keyringService.getSelectedAccount(selectedCoin)).address
          }

          const accountsRegistry: AccountInfoEntityState = await dispatch(walletApi.endpoints.getAccountInfosRegistry.initiate()).unwrap()
          const fallbackAccount = accountsRegistry[accountsRegistry.ids[0]]

          if (
            // If the selected address is null, set the selected account address to the fallback address
            selectedAddress === null || selectedAddress === '' ||
            // If a user has already created an wallet but then chooses to restore
            // a different wallet, getSelectedAccount still returns the previous wallets
            // selected account.
            // This check looks to see if the returned selectedAccount exist in the accountInfos
            // payload, if not it will setSelectedAccount to the fallback address
            !accountsRegistry.ids.find((accountId) => String(accountId).toLowerCase() === selectedAddress?.toLowerCase())
          ) {
            await dispatch(walletApi.endpoints.setSelectedAccount.initiate(fallbackAccount))
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
        async queryFn (arg, api, extraOptions, baseQuery) {
          try {
            const { braveWalletService } = baseQuery(undefined).data
            const { currency } = await braveWalletService.getDefaultBaseCurrency()
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
        async queryFn (currencyArg, api, extraOptions, baseQuery) {
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
      getHiddenNetworkChainIdsForCoin: query<string[], BraveWallet.CoinType>({
        async queryFn (coinTypeArg, api, extraOptions, baseQuery) {
          try {
            const { jsonRpcService } = baseQuery(undefined).data
            const { chainIds } = await jsonRpcService.getHiddenNetworks(coinTypeArg)
            return {
              data: chainIds
            }
          } catch (error) {
            return {
              error: `Unable to fetch HiddenNetworkChainIdsForCoin for coin: ${coinTypeArg}`
            }
          }
        }
      }),
      getAllNetworks: query<NetworkEntityState, void>({
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          try {
            const { jsonRpcService } = baseQuery(undefined).data

            // network type flags
            const {
              isFilecoinEnabled,
              isSolanaEnabled
            } = await dispatch(walletApi.endpoints.getWalletInfoBase.initiate()).unwrap()

            // Get all networks
            const filteredSupportedCoinTypes = SupportedCoinTypes.filter((coin) => {
              // MULTICHAIN: While we are still in development for FIL and SOL,
              // we will not use their networks unless enabled by brave://flags
              return (
                (coin === BraveWallet.CoinType.FIL && isFilecoinEnabled) ||
                (coin === BraveWallet.CoinType.SOL && isSolanaEnabled) ||
                coin === BraveWallet.CoinType.ETH
              )
            })

            // Get all networks for supported coin types
            const networkLists: BraveWallet.NetworkInfo[][] = await Promise.all(
              filteredSupportedCoinTypes.map(
                async (coin: BraveWallet.CoinType) => {
                  const { networks } = await jsonRpcService.getAllNetworks(coin)
                  const hiddenChains: string[] = await dispatch(
                    walletApi.endpoints.getHiddenNetworkChainIdsForCoin.initiate(coin)
                  ).unwrap()

                  return networks.filter((n) => !hiddenChains.includes(n.chainId))
                }
              )
            )
            const networksList = networkLists.flat(1)

            // normalize list into a registry
            const normalizedNetworksState = networkEntityAdapter.setAll(
              networkEntityInitalState,
              networksList
            )
            return {
              data: normalizedNetworksState
            }
          } catch (error) {
            return {
              error: `Unable to fetch AllNetworks ${error}`
            }
          }
        },
        providesTags: cacher.providesRegistry('Network')
      }),
      getChainIdForCoin: query<string, BraveWallet.CoinType>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          const { jsonRpcService } = baseQuery(undefined).data // apiProxy
          const { chainId } = await jsonRpcService.getChainId(arg)
          return {
            data: chainId
          }
        },
        providesTags: cacher.cacheByIdArg('ChainIdForCoinType')
      }),
      getSelectedChainId: query<string, void>({
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          const selectedCoin: number = await dispatch(walletApi.endpoints.getSelectedCoin.initiate()).unwrap()
          const chainId: string = await dispatch(walletApi.endpoints.getChainIdForCoin.initiate(selectedCoin)).unwrap()
          return {
            data: chainId
          }
        },
        providesTags: ['SelectedChainId']
      }),
      getSelectedCoin: query<BraveWallet.CoinType, void>({
        async queryFn (arg, api, extraOptions, baseQuery) {
          try {
            const apiProxy = baseQuery(undefined).data
            const { braveWalletService } = apiProxy
            const { coin } = await braveWalletService.getSelectedCoin()
            return { data: coin }
          } catch (error) {
            return {
              error: `Unable to fetch selectedCoin: ${error}`
            }
          }
        },
        providesTags: ['SelectedCoin']
      }),
      setSelectedCoin: mutation<BraveWallet.CoinType, BraveWallet.CoinType>({
        queryFn (coinTypeArg, api, extraOptions, baseQuery) {
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
        invalidatesTags: ['SelectedCoin']
      }),
      isEip1559Changed: mutation<{ id: string, isEip1559: boolean }, IsEip1559Changed>({
        async queryFn (arg) {
          const { chainId, isEip1559 } = arg
          // cache which chains are using EIP1559
          return {
            data: { id: chainId, isEip1559 } // invalidate the cache of the network with this chainId
          }
        },
        async onQueryStarted ({ chainId, isEip1559 }, { dispatch, queryFulfilled }) {
          // optimistic updates
          // try manually updating the cached network with the updated isEip1559 value
          const patchResult = dispatch(walletApi.util.updateQueryData('getAllNetworks', undefined, (draft) => {
            const draftNet = draft.entities[chainId]
            if (draftNet) {
              draftNet.isEip1559 = isEip1559
            }
          }))

          try {
            await queryFulfilled
          } catch {
            // undo the optimistic update if the mutation failed
            patchResult.undo()
          }
        },
        invalidatesTags: cacher.invalidatesList('Network')
      }),
      //
      // Prices
      //
      getTokenSpotPrice: query<AssetPriceById, GetBlockchainTokenIdArg>({
        async queryFn (tokenArg, { dispatch }, extraOptions, baseQuery) {
          try {
            const { assetRatioService } = baseQuery(undefined).data

            const defaultFiatCurrency = await dispatch(
              walletApi.endpoints.getDefaultFiatCurrency.initiate()
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
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          try {
            const { blockchainRegistry } = baseQuery(undefined).data
            const networksState: NetworkEntityState = await dispatch(
              walletApi.endpoints.getAllNetworks.initiate()
            ).unwrap()

            const networksList: BraveWallet.NetworkInfo[] =
              getEntitiesListFromEntityState(networksState)

            const tokenIdsByChainId: Record<string, string[]> = {}

            const tokenListsForNetworks = await Promise.all(
              networksList.map(async (network) => {
                const { tokens } = await blockchainRegistry.getAllTokens(network.chainId, network.coin)

                const fullTokensListForNetwork: BraveWallet.BlockchainToken[] = tokens.map(token => {
                  token.chainId = network.chainId
                  return addLogoToToken(token)
                })

                tokenIdsByChainId[network.chainId] =
                  fullTokensListForNetwork.map(getAssetIdKey)
                return fullTokensListForNetwork
              })
            )

            const flattendTokensList = tokenListsForNetworks.flat(1)

            if (flattendTokensList.length === 0) {
              throw new Error()
            }

            const tokensByChainIdRegistry = blockchainTokenEntityAdaptor.setAll(
              {
                ...blockchainTokenEntityAdaptorInitialState,
                idsByChainId: tokenIdsByChainId
              },
              flattendTokensList
            )

            return {
              data: tokensByChainIdRegistry
            }
          } catch (error) {
            return {
              error: 'Unable to fetch Tokens Registry'
            }
          }
        },
        providesTags: cacher.providesRegistry('KnownBlockchainTokens')
      }),
      getUserTokensRegistry: query<BlockchainTokenEntityAdaptorState, void>({
        async queryFn (arg, { dispatch }, extraOptions, baseQuery) {
          try {
            const { braveWalletService } = baseQuery(undefined).data
            const networksState: NetworkEntityState = await dispatch(
              walletApi.endpoints.getAllNetworks.initiate()
            ).unwrap()
            const networksList: BraveWallet.NetworkInfo[] =
              getEntitiesListFromEntityState(networksState)

            const tokenIdsByChainId: Record<string, string[]> = {}

            const userTokenListsForNetworks = await Promise.all(
              networksList.map(async (network) => {
                const fullTokensListForNetwork: BraveWallet.BlockchainToken[] =
                  await fetchUserAssetsForNetwork(braveWalletService, network)

                tokenIdsByChainId[network.chainId] =
                  fullTokensListForNetwork.map(getAssetIdKey)

                return fullTokensListForNetwork
              })
            )

            const userTokensByChainIdRegistry =
              blockchainTokenEntityAdaptor.setAll(
                {
                  ...blockchainTokenEntityAdaptorInitialState,
                  idsByChainId: tokenIdsByChainId
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
        providesTags: cacher.providesRegistry('UserBlockchainTokens')
      }),
      getERC721Metadata: query<{ id: EntityId, metadata?: ERC721Metadata }, GetBlockchainTokenIdArg>({
        async queryFn (tokenArg, api, extraOptions, baseQuery) {
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
            return { error: `error parsing erc721 metadata result: ${result.response}` }
          }
        },
        providesTags: (result, error, tokenQueryArg) => {
          const tag = {
            type: 'ERC721Metadata',
            id: blockchainTokenEntityAdaptor.selectId(tokenQueryArg)
          } as const

          return error ? [tag, 'UNKNOWN_ERROR'] : [tag]
        }
      }),
      addUserToken: mutation<{ id: EntityId }, BraveWallet.BlockchainToken>({
        async queryFn (tokenArg, { dispatch }, extraOptions, baseQuery) {
          const { braveWalletService } = baseQuery(undefined).data
          if (tokenArg.isErc721) {
            // Get NFTMetadata
            const { metadata } =
              await dispatch(walletApi.endpoints.getERC721Metadata.initiate({
                chainId: tokenArg.chainId,
                contractAddress: tokenArg.contractAddress,
                isErc721: tokenArg.isErc721,
                tokenId: tokenArg.tokenId,
                symbol: tokenArg.symbol
              })).unwrap()

            if (metadata?.image) {
              tokenArg.logo = metadata?.image || tokenArg.logo
            }
          }

          const result = await braveWalletService.addUserAsset(tokenArg)
          const tokenIdentifier = blockchainTokenEntityAdaptor.selectId(tokenArg)

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
        async queryFn (tokenArg, api, extraOptions, baseQuery) {
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
        async queryFn (tokenArg, { dispatch }, extraOptions, baseQuery) {
          const deleted = await dispatch(walletApi.endpoints.removeUserToken.initiate(tokenArg)).unwrap()
          if (deleted) {
            const result: { id: EntityId } = await dispatch(walletApi.endpoints.addUserToken.initiate(tokenArg)).unwrap()
            return { data: { id: result.id } }
          }
          return {
            error: `unable to update token ${getAssetIdKey(tokenArg)}`
          }
        },
        invalidatesTags: (_, __, tokenArg) => [{ type: 'UserBlockchainTokens', id: getAssetIdKey(tokenArg) }],
        async onQueryStarted (tokenArg, { queryFulfilled, dispatch }) {
          const patchResult = dispatch(walletApi.util.updateQueryData('getUserTokensRegistry', undefined, (draft) => {
            const tokenIdentifier = blockchainTokenEntityAdaptor.selectId(tokenArg)
            draft.entities[tokenIdentifier] = tokenArg
          }))
          try {
            await queryFulfilled
          } catch (error) {
            patchResult.undo()
          }
        }
      }),
      updateUserAssetVisible: mutation<boolean, SetUserAssetVisiblePayloadType>({
        async queryFn ({ isVisible, token }, api, extraOptions, baseQuery) {
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
                getAssetIdKey(token)
              }`
            }
          }
        },
        invalidatesTags: cacher.invalidatesList('UserBlockchainTokens')
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
  useGetAccountInfosRegistryQuery,
  useGetAllNetworksQuery,
  useGetChainIdForCoinQuery,
  useGetDefaultAccountAddressesQuery,
  useGetERC721MetadataQuery,
  useGetSelectedAccountAddressQuery,
  useGetSelectedChainIdQuery,
  useGetSelectedCoinQuery,
  useGetTokenSpotPriceQuery,
  useGetTokensRegistryQuery,
  useGetUserTokensRegistryQuery,
  useGetWalletInfoBaseQuery,
  useIsEip1559ChangedMutation,
  useLazyGetAccountInfosRegistryQuery,
  useLazyGetAllNetworksQuery,
  useLazyGetChainIdForCoinQuery,
  useLazyGetDefaultAccountAddressesQuery,
  useLazyGetERC721MetadataQuery,
  useLazyGetSelectedAccountAddressQuery,
  useLazyGetSelectedChainIdQuery,
  useLazyGetSelectedCoinQuery,
  useLazyGetTokenSpotPriceQuery,
  useLazyGetTokensRegistryQuery,
  useLazyGetUserTokensRegistryQuery,
  useLazyGetWalletInfoBaseQuery,
  usePrefetch,
  useSetSelectedAccountMutation,
  useSetSelectedCoinMutation
} = walletApi

export type WalletApiSliceState = ReturnType<typeof walletApi['reducer']>
export type WalletApiSliceStateFromRoot = { walletApi: WalletApiSliceState }

//
// Internals
//

async function fetchUserAssetsForNetwork (
  braveWalletService: BraveWallet.BraveWalletServiceRemote,
  network: BraveWallet.NetworkInfo
) {
  // Get a list of user tokens for each coinType and network.
  const getTokenList = await braveWalletService.getUserAssets(
    network.chainId,
    network.coin
  )

  // Adds a logo and chainId to each token object
  const tokenList: BraveWallet.BlockchainToken[] = getTokenList.tokens.map(token => {
    const updatedToken = addLogoToToken(token)
    updatedToken.chainId = network.chainId
    return updatedToken
  })

  if (tokenList.length === 0) {
    // Creates a network's Native Asset if nothing was returned
    const nativeAsset = makeNetworkAsset(network)
    nativeAsset.logo = network.iconUrls[0] ?? ''
    nativeAsset.visible = false
    return [nativeAsset]
  }

  return tokenList
}
