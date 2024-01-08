// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'
import { EntityId } from '@reduxjs/toolkit'

// types
import type WalletApiProxy from '../wallet_api_proxy'
import type { WalletPageApiProxy } from '../../page/wallet_page_api_proxy'
import type { WalletPanelApiProxy } from '../../panel/wallet_panel_api_proxy'

// constants
import {
  BraveWallet,
  SupportedCoinTypes,
  SupportedTestNetworks,
  SupportedOnRampNetworks,
  SupportedOffRampNetworks,
  ERC721Metadata,
  BraveRewardsInfo
} from '../../constants/types'

// entities
import {
  AccountInfoEntityState,
  accountInfoEntityAdaptor,
  accountInfoEntityAdaptorInitialState
} from '../slices/entities/account-info.entity'
import {
  BlockchainTokenEntityAdaptorState,
  blockchainTokenEntityAdaptor,
  blockchainTokenEntityAdaptorInitialState
} from '../slices/entities/blockchain-token.entity'
import {
  NetworksRegistry,
  networkEntityAdapter,
  emptyNetworksRegistry
} from '../slices/entities/network.entity'

// utils
import getAPIProxy from './bridge'
import {
  addChainIdToToken,
  getAssetIdKey,
  GetBlockchainTokenIdArg
} from '../../utils/asset-utils'
import { addLogoToToken } from './lib'
import { makeNetworkAsset } from '../../options/asset-options'
import { isIpfs } from '../../utils/string-utils'
import { getEnabledCoinTypes } from '../../utils/api-utils'
import {
  BraveRewardsProxy,
  WalletStatus,
  getBraveRewardsProxy
} from './brave_rewards_api_proxy'
import {
  getRewardsBATToken,
  getNormalizedExternalRewardsWallet,
  getNormalizedExternalRewardsNetwork,
  getRewardsProviderName
} from '../../utils/rewards_utils'

/**
 * A function to return the ref to either the main api proxy, or a mocked proxy
 * @returns function that returns an ApiProxy instance
 */
export let apiProxyFetcher = () =>
  getAPIProxy() as WalletApiProxy &
    Partial<WalletPanelApiProxy> &
    Partial<WalletPageApiProxy>

/**
 * Assigns a function to use for fetching the walletApiProxy
 * (useful for injecting spies during testing)
 * @param fetcher A function to return the ref to either the main api proxy,
 *  or a mocked proxy
 */
export const setApiProxyFetcher = (fetcher: () => WalletApiProxy) => {
  apiProxyFetcher = fetcher
}

/**
 * A function to return the ref to either the main api proxy, or a mocked proxy
 * @returns function that returns an ApiProxy instance
 */
export let rewardsProxyFetcher = getBraveRewardsProxy

/**
 * Assigns a function to use for fetching a BraveRewardsProxy
 * (useful for injecting spies during testing)
 * @param fetcher A function to return the ref to either the main api proxy,
 *  or a mocked proxy
 */
export const setRewardsProxyFetcher = (fetcher: () => BraveRewardsProxy) => {
  rewardsProxyFetcher = fetcher
}

/**
 * A place to store & manage dependency data for other queries
 */
export class BaseQueryCache {
  walletInfo?: BraveWallet.WalletInfo
  private _networksRegistry?: NetworksRegistry
  private _allAccountsInfo?: BraveWallet.AllAccountsInfo
  private _accountsRegistry?: AccountInfoEntityState
  private _userTokensRegistry?: BlockchainTokenEntityAdaptorState
  private _nftImageIpfsGateWayUrlRegistry: Record<string, string | null> = {}
  private _extractedIPFSUrlRegistry: Record<string, string | undefined> = {}
  private _enabledCoinTypes: number[]
  private _erc721MetadataRegistry: Record<string, ERC721Metadata>
  public rewardsInfo: BraveRewardsInfo | undefined = undefined

  getWalletInfo = async () => {
    if (!this.walletInfo) {
      const { walletInfo } =
        await apiProxyFetcher().walletHandler.getWalletInfo()
      this.walletInfo = walletInfo
    }
    return this.walletInfo
  }

  getAllAccounts = async () => {
    if (!this._allAccountsInfo) {
      const { allAccounts } =
        await apiProxyFetcher().keyringService.getAllAccounts()
      this._allAccountsInfo = allAccounts
    }
    return this._allAccountsInfo
  }

  getAccountsRegistry = async () => {
    if (!this._accountsRegistry) {
      const allAccounts = await this.getAllAccounts()

      this._accountsRegistry = accountInfoEntityAdaptor.setAll(
        accountInfoEntityAdaptorInitialState,
        allAccounts.accounts
      )
    }
    return this._accountsRegistry
  }

  clearWalletInfo = () => {
    this.walletInfo = undefined
    this._allAccountsInfo = undefined
    this._accountsRegistry = undefined
  }

  clearAccountsRegistry = () => {
    this.clearWalletInfo()
  }

  clearSelectedAccount = () => {
    this.clearAccountsRegistry()
  }

  getNetworksRegistry = async () => {
    if (!this._networksRegistry) {
      const { jsonRpcService } = apiProxyFetcher()

      // network type flags
      const { isBitcoinEnabled, isZCashEnabled } = await this.getWalletInfo()

      // Get all networks
      const filteredSupportedCoinTypes = SupportedCoinTypes.filter((coin) => {
        // FIL and SOL networks, unless enabled by brave://flags
        return (
          coin === BraveWallet.CoinType.FIL ||
          coin === BraveWallet.CoinType.SOL ||
          (coin === BraveWallet.CoinType.BTC && isBitcoinEnabled) ||
          (coin === BraveWallet.CoinType.ZEC && isZCashEnabled) ||
          coin === BraveWallet.CoinType.ETH
        )
      })

      const visibleIds: string[] = []
      const hiddenIds: string[] = []
      const idsByCoinType: Record<EntityId, EntityId[]> = {}
      const hiddenIdsByCoinType: Record<EntityId, string[]> = {}
      const mainnetIds: string[] = []
      const testnetIds: string[] = []
      const onRampIds: string[] = []
      const offRampIds: string[] = []

      // Get all networks for supported coin types
      const networkLists: BraveWallet.NetworkInfo[][] = await mapLimit(
        filteredSupportedCoinTypes,
        10,
        async (coin: BraveWallet.CoinType) => {
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

            if (SupportedTestNetworks.includes(chainId)) {
              testnetIds.push(networkId)
            } else {
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
        }
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
          mainnetIds,
          testnetIds
        },
        networksList
      )

      this._networksRegistry = normalizedNetworksState
    }
    return this._networksRegistry
  }

  clearNetworksRegistry = () => {
    this._networksRegistry = undefined
  }

  getUserTokensRegistry = async () => {
    if (!this._userTokensRegistry) {
      const networksRegistry = await this.getNetworksRegistry()

      const userTokensByChainIdRegistry = await makeTokensRegistry(
        networksRegistry,
        'user'
      )

      this._userTokensRegistry = userTokensByChainIdRegistry
    }
    return this._userTokensRegistry
  }

  clearUserTokensRegistry = () => {
    this._userTokensRegistry = undefined
  }

  /** Extracts ipfs:// url from gateway-like url */
  getExtractedIPFSUrlFromGatewayLikeUrl = async (urlArg: string) => {
    const trimmedURL = urlArg ? urlArg.trim() : ''
    if (!this._extractedIPFSUrlRegistry[trimmedURL]) {
      if (isIpfs(trimmedURL)) {
        this._extractedIPFSUrlRegistry[trimmedURL] = trimmedURL
      } else {
        const api = apiProxyFetcher()
        const { ipfsUrl } =
          await api.braveWalletIpfsService.extractIPFSUrlFromGatewayLikeUrl(
            trimmedURL
          )
        this._extractedIPFSUrlRegistry[trimmedURL] = ipfsUrl || undefined
      }
    }

    return this._extractedIPFSUrlRegistry[trimmedURL]
  }

  /** Translates ipfs:// url or gateway-like url to the NFT gateway url */
  getIpfsGatewayTranslatedNftUrl = async (urlArg: string) => {
    const trimmedURL = urlArg.trim()

    if (!this._nftImageIpfsGateWayUrlRegistry[trimmedURL]) {
      const { braveWalletIpfsService } = apiProxyFetcher()

      const testUrl = isIpfs(trimmedURL)
        ? trimmedURL
        : await this.getExtractedIPFSUrlFromGatewayLikeUrl(trimmedURL)

      const { translatedUrl } =
        await braveWalletIpfsService.translateToNFTGatewayURL(testUrl || '')

      this._nftImageIpfsGateWayUrlRegistry[trimmedURL] =
        translatedUrl || trimmedURL
    }

    return this._nftImageIpfsGateWayUrlRegistry[trimmedURL]
  }

  getEnabledCoinTypes = async () => {
    if (!this._enabledCoinTypes || !this._enabledCoinTypes.length) {
      // network type flags
      this._enabledCoinTypes = await getEnabledCoinTypes(apiProxyFetcher())
    }

    return this._enabledCoinTypes
  }

  getErc721Metadata = async (tokenArg: GetBlockchainTokenIdArg) => {
    if (!tokenArg.isErc721) {
      throw new Error('Cannot fetch erc-721 metadata for non erc-721 token')
    }

    const tokenId = blockchainTokenEntityAdaptor.selectId(tokenArg)

    if (!this._erc721MetadataRegistry[tokenId]) {
      const { jsonRpcService } = apiProxyFetcher()

      const result = await jsonRpcService.getERC721Metadata(
        tokenArg.contractAddress,
        tokenArg.tokenId,
        tokenArg.chainId
      )

      if (result.error || result.errorMessage) {
        throw new Error(result.errorMessage)
      }

      const metadata: ERC721Metadata = JSON.parse(result.response)

      this._erc721MetadataRegistry[tokenId] = metadata
    }

    return this._erc721MetadataRegistry[tokenId]
  }

  // Brave Rewards
  getBraveRewardsInfo = async () => {
    if (!this.rewardsInfo) {
      const isRewardsEnabled = await rewardsProxyFetcher().getRewardsEnabled()

      if (!isRewardsEnabled) {
        this.rewardsInfo = emptyRewardsInfo
        return this.rewardsInfo
      }

      const balance = await rewardsProxyFetcher().fetchBalance()

      const { provider, status, links } =
        (await rewardsProxyFetcher().getExternalWallet()) || {}

      this.rewardsInfo = {
        isRewardsEnabled: true,
        balance,
        provider,
        status: status || WalletStatus.kNotConnected,
        accountLink: links?.account,
        rewardsToken: getRewardsBATToken(provider),
        rewardsAccount: getNormalizedExternalRewardsWallet(provider),
        rewardsNetwork: getNormalizedExternalRewardsNetwork(provider),
        providerName: getRewardsProviderName(provider)
      }
    }

    return this.rewardsInfo
  }
}

let cache = new BaseQueryCache()

export const baseQueryFunction = () => {
  if (!cache) {
    cache = new BaseQueryCache()
  }
  return { data: apiProxyFetcher(), cache: cache }
}

export const resetCache = () => {
  cache = new BaseQueryCache()
}

type AssetsListType = 'user' | 'known'

// internals
async function fetchAssetsForNetwork(
  listType: AssetsListType,
  network: BraveWallet.NetworkInfo
) {
  const { blockchainRegistry, braveWalletService } = apiProxyFetcher()
  // Get a list of user tokens for each coinType and network.
  const { tokens } =
    listType === 'known'
      ? await blockchainRegistry.getAllTokens(network.chainId, network.coin)
      : await braveWalletService.getUserAssets(network.chainId, network.coin)

  // Adds a logo and chainId to each token object
  const tokenList: BraveWallet.BlockchainToken[] = await mapLimit(
    tokens,
    10,
    async (token: BraveWallet.BlockchainToken) => {
      const updatedToken = await addLogoToToken(token)
      return addChainIdToToken(updatedToken, network.chainId)
    }
  )

  if (tokenList.length === 0) {
    // Creates a network's Native Asset if nothing was returned
    const nativeAsset = makeNetworkAsset(network)
    nativeAsset.logo = network.iconUrls[0] ?? ''
    nativeAsset.visible = false
    return [nativeAsset]
  }

  return tokenList
}

export async function makeTokensRegistry(
  networksRegistry: NetworksRegistry,
  listType: AssetsListType
) {
  const nonFungibleTokenIds: string[] = []
  const fungibleTokenIds: string[] = []

  const idsByChainId: Record<string, string[]> = {}
  const idsByCoinType: Record<BraveWallet.CoinType, string[]> = {}
  const visibleTokenIds: string[] = []
  const hiddenTokenIds: string[] = []
  const visibleTokenIdsByChainId: Record<string, string[]> = {}
  const hiddenTokenIdsByChainId: Record<string, string[]> = {}
  const visibleTokenIdsByCoinType: Record<BraveWallet.CoinType, string[]> = {}
  const hiddenTokenIdsByCoinType: Record<BraveWallet.CoinType, string[]> = {}

  const fungibleIdsByChainId: Record<string, string[]> = {}
  const fungibleIdsByCoinType: Record<BraveWallet.CoinType, string[]> = {}
  const fungibleVisibleTokenIds: string[] = []
  const fungibleHiddenTokenIds: string[] = []
  const fungibleVisibleTokenIdsByChainId: Record<string, string[]> = {}
  const fungibleHiddenTokenIdsByChainId: Record<string, string[]> = {}
  const fungibleVisibleTokenIdsByCoinType: Record<
    BraveWallet.CoinType,
    string[]
  > = {}
  const fungibleHiddenTokenIdsByCoinType: Record<
    BraveWallet.CoinType,
    string[]
  > = {}

  const nonFungibleIdsByChainId: Record<string, string[]> = {}
  const nonFungibleIdsByCoinType: Record<BraveWallet.CoinType, string[]> = {}
  const nonFungibleVisibleTokenIds: string[] = []
  const nonFungibleHiddenTokenIds: string[] = []
  const nonFungibleVisibleTokenIdsByChainId: Record<string, string[]> = {}
  const nonFungibleHiddenTokenIdsByChainId: Record<string, string[]> = {}
  const nonFungibleVisibleTokenIdsByCoinType: Record<
    BraveWallet.CoinType,
    string[]
  > = {}
  const nonFungibleHiddenTokenIdsByCoinType: Record<
    BraveWallet.CoinType,
    string[]
  > = {}

  const userTokenListsForNetworks = await mapLimit(
    Object.entries(networksRegistry.entities),
    10,
    async ([networkId, network]: [string, BraveWallet.NetworkInfo]) => {
      if (!network) {
        return []
      }

      const fullTokensListForNetwork: BraveWallet.BlockchainToken[] =
        await fetchAssetsForNetwork(listType, network)

      idsByChainId[networkId] = []
      visibleTokenIdsByChainId[networkId] = []
      hiddenTokenIdsByChainId[networkId] = []
      fungibleIdsByChainId[networkId] = []
      fungibleVisibleTokenIdsByChainId[networkId] = []
      fungibleHiddenTokenIdsByChainId[networkId] = []
      nonFungibleIdsByChainId[networkId] = []
      nonFungibleVisibleTokenIdsByChainId[networkId] = []
      nonFungibleHiddenTokenIdsByChainId[networkId] = []
      for (const token of fullTokensListForNetwork) {
        const tokenId = getAssetIdKey(token)
        const { visible } = token
        const isNft = token.isNft || token.isErc1155 || token.isErc721

        idsByChainId[networkId].push(tokenId)
        if (isNft) {
          nonFungibleTokenIds.push(tokenId)
          nonFungibleIdsByChainId[networkId].push(tokenId)
        } else {
          fungibleTokenIds.push(tokenId)
          fungibleIdsByChainId[networkId].push(tokenId)
        }

        if (visible) {
          visibleTokenIdsByChainId[networkId].push(tokenId)
          if (isNft) {
            nonFungibleVisibleTokenIdsByChainId[networkId].push(tokenId)
          } else {
            fungibleVisibleTokenIdsByChainId[networkId].push(tokenId)
          }
        } else {
          hiddenTokenIdsByChainId[networkId].push(tokenId)
          if (isNft) {
            nonFungibleHiddenTokenIdsByChainId[networkId].push(tokenId)
          } else {
            fungibleHiddenTokenIdsByChainId[networkId].push(tokenId)
          }
        }
      }

      // All Ids by coin type
      idsByCoinType[network.coin] = (idsByCoinType[network.coin] || []).concat(
        idsByChainId[networkId]
      )

      nonFungibleIdsByCoinType[network.coin] = (
        nonFungibleIdsByCoinType[network.coin] || []
      ).concat(nonFungibleIdsByChainId[networkId])

      fungibleIdsByCoinType[network.coin] = (
        fungibleIdsByCoinType[network.coin] || []
      ).concat(fungibleIdsByChainId[networkId])

      // visible Ids by coin
      visibleTokenIdsByCoinType[network.coin] = (
        visibleTokenIdsByCoinType[network.coin] || []
      ).concat(visibleTokenIdsByChainId[networkId])

      nonFungibleVisibleTokenIdsByCoinType[network.coin] = (
        nonFungibleVisibleTokenIdsByCoinType[network.coin] || []
      ).concat(nonFungibleVisibleTokenIdsByChainId[networkId])

      fungibleVisibleTokenIdsByCoinType[network.coin] = (
        fungibleVisibleTokenIdsByCoinType[network.coin] || []
      ).concat(fungibleVisibleTokenIdsByChainId[networkId])

      // hidden Ids by coin
      hiddenTokenIdsByCoinType[network.coin] = (
        hiddenTokenIdsByCoinType[network.coin] || []
      ).concat(hiddenTokenIdsByChainId[networkId])

      nonFungibleHiddenTokenIdsByCoinType[network.coin] = (
        nonFungibleHiddenTokenIdsByCoinType[network.coin] || []
      ).concat(nonFungibleHiddenTokenIdsByChainId[networkId])

      fungibleHiddenTokenIdsByCoinType[network.coin] = (
        fungibleHiddenTokenIdsByCoinType[network.coin] || []
      ).concat(fungibleHiddenTokenIdsByChainId[networkId])

      // All visible ids
      visibleTokenIds.push(...visibleTokenIdsByChainId[networkId])
      nonFungibleVisibleTokenIds.push(
        ...nonFungibleVisibleTokenIdsByChainId[networkId]
      )
      fungibleVisibleTokenIds.push(
        ...fungibleVisibleTokenIdsByChainId[networkId]
      )

      // All hidden ids
      hiddenTokenIds.push(...hiddenTokenIdsByChainId[networkId])
      nonFungibleHiddenTokenIds.push(
        ...nonFungibleHiddenTokenIdsByChainId[networkId]
      )
      fungibleHiddenTokenIds.push(...fungibleHiddenTokenIdsByChainId[networkId])

      return fullTokensListForNetwork
    }
  )

  const userTokensByChainIdRegistry = blockchainTokenEntityAdaptor.setAll(
    {
      ...blockchainTokenEntityAdaptorInitialState,
      idsByChainId,
      visibleTokenIds,
      hiddenTokenIds,
      visibleTokenIdsByChainId,
      visibleTokenIdsByCoinType,
      idsByCoinType,

      fungibleHiddenTokenIds,
      fungibleTokenIds,
      fungibleIdsByChainId,
      fungibleIdsByCoinType,
      fungibleVisibleTokenIds,
      fungibleVisibleTokenIdsByChainId,
      fungibleVisibleTokenIdsByCoinType,

      nonFungibleHiddenTokenIds,
      nonFungibleTokenIds,
      nonFungibleIdsByChainId,
      nonFungibleIdsByCoinType,
      nonFungibleVisibleTokenIds,
      nonFungibleVisibleTokenIdsByChainId,
      nonFungibleVisibleTokenIdsByCoinType
    },
    userTokenListsForNetworks.flat(1)
  )
  return userTokensByChainIdRegistry
}

// defaults
export const emptyRewardsInfo: BraveRewardsInfo = {
  isRewardsEnabled: false,
  balance: undefined,
  rewardsToken: undefined,
  provider: undefined,
  providerName: '',
  status: WalletStatus.kNotConnected,
  rewardsAccount: undefined,
  rewardsNetwork: undefined,
  accountLink: undefined
} as const
