// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'
import { EntityId } from '@reduxjs/toolkit'

// constants
import {
  BraveWallet,
  SupportedCoinTypes,
  SupportedTestNetworks,
  SupportedOnRampNetworks,
  SupportedOffRampNetworks,
  BraveRewardsInfo,
  WalletStatus,
  NFTMetadataReturnType
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
  addLogoToToken,
  getAssetIdKey,
  GetBlockchainTokenIdArg,
  getDeletedTokenIds,
  getHiddenTokenIds,
  isNativeAsset
} from '../../utils/asset-utils'
import {
  makeNativeAssetLogo,
  makeNetworkAsset
} from '../../options/asset-options'
import { isIpfs } from '../../utils/string-utils'
import { getEnabledCoinTypes } from '../../utils/api-utils'
import { getBraveRewardsProxy } from './brave_rewards_api_proxy'
import {
  getRewardsBATToken,
  getNormalizedExternalRewardsWallet,
  getNormalizedExternalRewardsNetwork,
  getRewardsProviderName
} from '../../utils/rewards_utils'

/**
 * A place to store & manage dependency data for other queries
 */
export class BaseQueryCache {
  walletInfo?: BraveWallet.WalletInfo
  private _networksRegistry?: NetworksRegistry
  private _allAccountsInfo?: BraveWallet.AllAccountsInfo
  private _accountsRegistry?: AccountInfoEntityState
  private _knownTokensRegistry?: BlockchainTokenEntityAdaptorState
  private _userTokensRegistry?: BlockchainTokenEntityAdaptorState
  private _nftImageIpfsGateWayUrlRegistry: Record<string, string | null> = {}
  private _enabledCoinTypes: number[]
  private _nftMetadataRegistry: Record<string, NFTMetadataReturnType> = {}
  public rewardsInfo: BraveRewardsInfo | undefined = undefined
  public balanceScannerSupportedChains: string[] | undefined = undefined
  public spamNftsForAccountRegistry: Record<
    string, // accountUniqueId
    BraveWallet.BlockchainToken[]
  > = {}

  getWalletInfo = async () => {
    if (!this.walletInfo) {
      const { walletInfo } = await getAPIProxy().walletHandler.getWalletInfo()
      this.walletInfo = walletInfo
    }
    return this.walletInfo
  }

  getAllAccounts = async () => {
    if (!this._allAccountsInfo) {
      const { allAccounts } =
        await getAPIProxy().keyringService.getAllAccounts()
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
      const { jsonRpcService } = getAPIProxy()

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
      const visibleIdsByCoinType: Record<EntityId, EntityId[]> = {}
      const hiddenIdsByCoinType: Record<EntityId, string[]> = {}
      const mainnetIds: string[] = []
      const testnetIds: string[] = []
      const onRampIds: string[] = []
      const offRampIds: string[] = []

      const { networks } = await jsonRpcService.getAllNetworks()

      // Get all networks for supported coin types
      const networkLists: BraveWallet.NetworkInfo[][] = await mapLimit(
        filteredSupportedCoinTypes,
        10,
        async (coin: BraveWallet.CoinType) => {
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

          visibleIdsByCoinType[coin] = []
          hiddenIdsByCoinType[coin] = []

          networks.forEach(({ chainId, coin: networkCoin }) => {
            if (networkCoin !== coin) {
              return
            }
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
              visibleIdsByCoinType[coin].push(networkId)
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
          visibleIdsByCoinType,
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

  getBalanceScannerSupportedChains = async () => {
    if (!this.balanceScannerSupportedChains) {
      const { braveWalletService } = getAPIProxy()
      const { chainIds } =
        await braveWalletService.getBalanceScannerSupportedChains()
      this.balanceScannerSupportedChains = chainIds
    }
    return this.balanceScannerSupportedChains
  }

  getKnownTokensRegistry = async () => {
    if (!this._knownTokensRegistry) {
      const networksRegistry = await this.getNetworksRegistry()
      this._knownTokensRegistry = await makeTokensRegistry({
        networksRegistry,
        listType: 'known',
        cache
      })
    }
    return this._knownTokensRegistry
  }

  getUserTokensRegistry = async () => {
    if (!this._userTokensRegistry) {
      const networksRegistry = await this.getNetworksRegistry()
      this._userTokensRegistry = await makeTokensRegistry({
        networksRegistry,
        listType: 'user',
        cache
      })
    }
    return this._userTokensRegistry
  }

  clearKnownTokensRegistry = () => {
    this._knownTokensRegistry = undefined
  }

  clearUserTokensRegistry = () => {
    this._userTokensRegistry = undefined
  }

  /** Translates ipfs:// url or gateway-like url to the NFT gateway url */
  getIpfsGatewayTranslatedNftUrl = async (urlArg: string) => {
    const trimmedURL = urlArg.trim()
    if (!this._nftImageIpfsGateWayUrlRegistry[trimmedURL]) {
      const { braveWalletIpfsService } = getAPIProxy()

      const { translatedUrl } = isIpfs(trimmedURL)
        ? await braveWalletIpfsService.translateToGatewayURL(trimmedURL || '')
        : { translatedUrl: trimmedURL }

      this._nftImageIpfsGateWayUrlRegistry[trimmedURL] =
        translatedUrl || trimmedURL
    }

    return this._nftImageIpfsGateWayUrlRegistry[trimmedURL]
  }

  // TODO(apaymyshev): This function should not exist. Backend should be
  // responsible in providing correct logo.
  /** only caches ipfs translations since saving to a registry would require a
   * long identifier */
  getTokenLogo = async (token: BraveWallet.BlockchainToken) => {
    if (isNativeAsset(token)) {
      return makeNativeAssetLogo(token.symbol, token.chainId)
    }

    if (
      !token.logo ||
      token.logo.startsWith('data:image/') ||
      token.logo.startsWith('chrome://erc-token-images/')
    ) {
      // nothing to change
      return token.logo
    }
    return token.logo.startsWith('ipfs://')
      ? (await this.getIpfsGatewayTranslatedNftUrl(token.logo)) || ''
      : `chrome://erc-token-images/${token.logo}`
  }

  getEnabledCoinTypes = async () => {
    if (!this._enabledCoinTypes || !this._enabledCoinTypes.length) {
      // network type flags
      this._enabledCoinTypes = await getEnabledCoinTypes(getAPIProxy())
    }

    return this._enabledCoinTypes
  }

  getNftMetadata = async (tokenArg: GetBlockchainTokenIdArg) => {
    if (!tokenArg.isErc721 && !tokenArg.isNft) {
      throw new Error('Only NFTs are supported for metadata lookups')
    }

    if (
      tokenArg.coin !== BraveWallet.CoinType.ETH &&
      tokenArg.coin !== BraveWallet.CoinType.SOL
    ) {
      throw new Error(
        `Unsupported coin type for NFT metadata lookup ${tokenArg.coin}`
      )
    }

    const tokenId = blockchainTokenEntityAdaptor.selectId(tokenArg)

    if (!this._nftMetadataRegistry[tokenId]) {
      const { jsonRpcService } = getAPIProxy()

      const lookupArg = {
        chainId: tokenArg.chainId,
        contractAddress: tokenArg.contractAddress,
        tokenId: tokenArg.tokenId
      }

      const result = await jsonRpcService.getNftMetadatas(tokenArg.coin, [
        lookupArg
      ])
      if (result.errorMessage) {
        throw new Error(result.errorMessage)
      }

      if (!result?.metadatas?.length) {
        throw new Error(`Failed to get NFT metadata for token: ${tokenId}`)
      }

      const metadata: BraveWallet.NftMetadata = result.metadatas[0]

      const tokenNetwork = (await cache.getNetworksRegistry()).entities[
        networkEntityAdapter.selectId(tokenArg)
      ]

      const nftMetadata: NFTMetadataReturnType = {
        metadataUrl: '',
        chainName: tokenNetwork?.chainName || '',
        tokenType:
          tokenArg.coin === BraveWallet.CoinType.ETH
            ? tokenArg.isErc721
              ? 'ERC721'
              : 'ERC1155'
            : tokenArg.coin === BraveWallet.CoinType.SOL
            ? 'SPL'
            : '',
        tokenID: tokenArg.tokenId,
        imageURL: metadata.animationUrl || metadata.image || undefined,
        imageMimeType: 'image/*',
        floorFiatPrice: '',
        floorCryptoPrice: '',
        contractInformation: {
          address: tokenArg.contractAddress,
          name: metadata?.name || '???',
          description: metadata?.description || '???',
          website: '',
          facebook: '',
          logo: '',
          twitter: ''
        },
        collection:
          (metadata?.collection && { name: metadata.collection }) || undefined,
        attributes: metadata.attributes
      }

      this._nftMetadataRegistry[tokenId] = nftMetadata
    }

    return this._nftMetadataRegistry[tokenId]
  }

  getSpamNftsForAccountId = async (accountId: BraveWallet.AccountId) => {
    if (!this.spamNftsForAccountRegistry[accountId.uniqueKey]) {
      const { braveWalletService } = getAPIProxy()
      const { address, coin } = accountId
      const networksRegistry = await cache.getNetworksRegistry()

      const chainIds = networksRegistry.ids.map(
        (network) => networksRegistry.entities[network]!.chainId
      )

      let currentCursor: string | null = null
      const accountSpamNfts = []

      do {
        const {
          tokens,
          cursor
        }: {
          tokens: BraveWallet.BlockchainToken[]
          cursor: string | null
        } = await braveWalletService.getSimpleHashSpamNFTs(
          address,
          chainIds,
          coin,
          currentCursor
        )

        accountSpamNfts.push(...tokens)
        currentCursor = cursor
      } while (currentCursor)

      this.spamNftsForAccountRegistry[accountId.uniqueKey] = accountSpamNfts
    }

    return this.spamNftsForAccountRegistry[accountId.uniqueKey]
  }

  // Brave Rewards
  getBraveRewardsInfo = async () => {
    if (!this.rewardsInfo) {
      const isRewardsEnabled = await getBraveRewardsProxy().getRewardsEnabled()

      if (!isRewardsEnabled) {
        this.rewardsInfo = emptyRewardsInfo
        return this.rewardsInfo
      }
      const { provider, status, url } =
        (await getBraveRewardsProxy().getExternalWallet()) || {}

      if (!provider || provider === 'solana') {
        return emptyRewardsInfo
      }

      const balance = await getBraveRewardsProxy().fetchBalance()

      this.rewardsInfo = {
        isRewardsEnabled: true,
        balance,
        provider,
        status: status || WalletStatus.kNotConnected,
        accountLink: url,
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
  return { data: getAPIProxy(), cache }
}

export const resetCache = () => {
  cache = new BaseQueryCache()
}

type AssetsListType = 'user' | 'known'

// internals
async function fetchAssetsForNetwork({
  cache,
  listType,
  network
}: {
  listType: AssetsListType
  network: BraveWallet.NetworkInfo
  cache: BaseQueryCache
}) {
  const { blockchainRegistry, braveWalletService } = getAPIProxy()
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
      const tokenLogo = await cache.getTokenLogo(token)
      const updatedToken = addLogoToToken(token, tokenLogo)
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

export async function makeTokensRegistry({
  cache,
  listType,
  networksRegistry
}: {
  networksRegistry: NetworksRegistry
  listType: AssetsListType
  cache: BaseQueryCache
}) {
  const locallyDeletedTokenIds: string[] =
    listType === 'user' ? getDeletedTokenIds() : []
  const locallyHiddenTokenIds: string[] =
    listType === 'user' ? getHiddenTokenIds() : []
  const locallyRemovedTokenIds = locallyDeletedTokenIds.concat(
    locallyHiddenTokenIds
  )

  const nonFungibleTokenIds: string[] = []
  const fungibleTokenIds: string[] = []

  const idsByChainId: Record<string, string[]> = {}
  const idsByCoinType: Record<BraveWallet.CoinType, string[]> = {}
  const visibleTokenIds: string[] = []
  const visibleTokenIdsByChainId: Record<string, string[]> = {}
  const hiddenTokenIdsByChainId: Record<string, string[]> = {}
  const visibleTokenIdsByCoinType: Record<BraveWallet.CoinType, string[]> = {}
  const hiddenTokenIdsByCoinType: Record<BraveWallet.CoinType, string[]> = {}

  const deletedTokenIds: string[] = locallyDeletedTokenIds
  const hiddenTokenIds: string[] = []
  const spamTokenIds: string[] = []
  const nonSpamTokenIds: string[] = []

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
        await fetchAssetsForNetwork({ listType, network, cache })

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
        const isHidden = !visible || locallyRemovedTokenIds.includes(tokenId)

        idsByChainId[networkId].push(tokenId)

        if (token.isSpam) {
          spamTokenIds.push(tokenId)
        } else {
          nonSpamTokenIds.push(tokenId)
        }

        if (isNft) {
          nonFungibleTokenIds.push(tokenId)
          nonFungibleIdsByChainId[networkId].push(tokenId)
          if (isHidden) {
            hiddenTokenIdsByChainId[networkId].push(tokenId)
            nonFungibleHiddenTokenIdsByChainId[networkId].push(tokenId)
          } else {
            visibleTokenIdsByChainId[networkId].push(tokenId)
            nonFungibleVisibleTokenIdsByChainId[networkId].push(tokenId)
          }
        } else {
          fungibleTokenIds.push(tokenId)
          fungibleIdsByChainId[networkId].push(tokenId)
          if (isHidden) {
            hiddenTokenIdsByChainId[networkId].push(tokenId)
            fungibleHiddenTokenIdsByChainId[networkId].push(tokenId)
          } else {
            visibleTokenIdsByChainId[networkId].push(tokenId)
            fungibleVisibleTokenIdsByChainId[networkId].push(tokenId)
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
      deletedTokenIds,
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
      nonFungibleVisibleTokenIdsByCoinType,

      spamTokenIds,
      nonSpamTokenIds
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
