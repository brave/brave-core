// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  createDraftSafeSelector,
  createEntityAdapter,
  EntityAdapter,
  EntityId
} from '@reduxjs/toolkit'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import {
  getAssetIdKey,
  GetBlockchainTokenIdArg
} from '../../../utils/asset-utils'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import { walletApi, WalletApiSliceStateFromRoot } from '../api.slice'

export type BlockchainTokenEntityAdaptor =
  EntityAdapter<BraveWallet.BlockchainToken> & {
    selectId: (t: GetBlockchainTokenIdArg) => EntityId
  }

export const blockchainTokenEntityAdaptor: BlockchainTokenEntityAdaptor =
  createEntityAdapter<BraveWallet.BlockchainToken>({
    selectId: getAssetIdKey
  })
export type BlockchainTokenEntityAdaptorState = ReturnType<
  BlockchainTokenEntityAdaptor['getInitialState']
> & {
  // all tokens
  idsByChainId: Record<EntityId, EntityId[]>
  idsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  visibleTokenIds: string[]
  hiddenTokenIds: string[]
  deletedTokenIds: string[]
  visibleTokenIdsByChainId: Record<string, string[]>
  visibleTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  hiddenTokenIdsByChainId: Record<string, string[]>
  hiddenTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>

  // fungible tokens
  fungibleIdsByChainId: Record<EntityId, EntityId[]>
  fungibleIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  fungibleTokenIds: string[]
  fungibleVisibleTokenIds: string[]
  fungibleHiddenTokenIds: string[]
  fungibleVisibleTokenIdsByChainId: Record<string, string[]>
  fungibleVisibleTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  fungibleHiddenTokenIdsByChainId: Record<string, string[]>
  fungibleHiddenTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>

  // non-fungible tokens
  nonFungibleIdsByChainId: Record<EntityId, EntityId[]>
  nonFungibleIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  nonFungibleTokenIds: string[]
  nonFungibleVisibleTokenIds: string[]
  nonFungibleHiddenTokenIds: string[]
  nonFungibleVisibleTokenIdsByChainId: Record<string, string[]>
  nonFungibleVisibleTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  nonFungibleHiddenTokenIdsByChainId: Record<string, string[]>
  nonFungibleHiddenTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>

  // spam
  spamTokenIds: string[]
  nonSpamTokenIds: string[]
}

export const blockchainTokenEntityAdaptorInitialState: //
BlockchainTokenEntityAdaptorState = {
  ...blockchainTokenEntityAdaptor.getInitialState(),
  idsByChainId: {},
  idsByCoinType: {},
  visibleTokenIds: [],
  hiddenTokenIds: [],
  deletedTokenIds: [],
  visibleTokenIdsByChainId: {},
  visibleTokenIdsByCoinType: {},
  hiddenTokenIdsByChainId: {},
  hiddenTokenIdsByCoinType: {},

  fungibleIdsByChainId: {},
  fungibleIdsByCoinType: {},
  fungibleTokenIds: [],
  fungibleHiddenTokenIds: [],
  fungibleVisibleTokenIds: [],
  fungibleVisibleTokenIdsByChainId: {},
  fungibleVisibleTokenIdsByCoinType: {},
  fungibleHiddenTokenIdsByChainId: {},
  fungibleHiddenTokenIdsByCoinType: {},

  nonFungibleIdsByChainId: {},
  nonFungibleIdsByCoinType: {},
  nonFungibleTokenIds: [],
  nonFungibleHiddenTokenIds: [],
  nonFungibleVisibleTokenIds: [],
  nonFungibleVisibleTokenIdsByChainId: {},
  nonFungibleVisibleTokenIdsByCoinType: {},
  nonFungibleHiddenTokenIdsByChainId: {},
  nonFungibleHiddenTokenIdsByCoinType: {},

  spamTokenIds: [],
  nonSpamTokenIds: []
}

export const combineTokenRegistries = (
  tokensRegistry: BlockchainTokenEntityAdaptorState,
  userTokensRegistry: BlockchainTokenEntityAdaptorState
): BlockchainTokenEntityAdaptorState => {
  const chainIds = new Set(
    Object.keys(tokensRegistry.idsByChainId).concat(
      Object.keys(userTokensRegistry.idsByChainId)
    )
  )
  const coinTypes = new Set(
    Object.keys(tokensRegistry.idsByCoinType).concat(
      Object.keys(userTokensRegistry.idsByCoinType)
    )
  )

  const idsByCoinType: Record<string, string[]> = {}
  const fungibleIdsByCoinType: Record<string, string[]> = {}
  const nonFungibleIdsByCoinType: Record<string, string[]> = {}
  const visibleTokenIdsByCoinType: Record<number, EntityId[]> = {}
  const hiddenTokenIdsByCoinType: Record<number, EntityId[]> = {}
  const fungibleVisibleTokenIdsByCoinType: Record<number, EntityId[]> = {}
  const nonFungibleVisibleTokenIdsByCoinType: Record<number, EntityId[]> = {}
  const fungibleHiddenTokenIdsByCoinType: Record<number, EntityId[]> = {}
  const nonFungibleHiddenTokenIdsByCoinType: Record<number, EntityId[]> = {}
  for (const coinType of coinTypes) {
    // ids by coin type
    idsByCoinType[coinType] = Array.from(
      new Set(
        (idsByCoinType[coinType] || [])
          .concat(tokensRegistry.idsByCoinType[coinType])
          .concat(userTokensRegistry.idsByCoinType[coinType])
      )
    )
    fungibleIdsByCoinType[coinType] = Array.from(
      new Set(
        (fungibleIdsByCoinType[coinType] || [])
          .concat(tokensRegistry.fungibleIdsByCoinType[coinType])
          .concat(userTokensRegistry.fungibleIdsByCoinType[coinType])
      )
    )
    nonFungibleIdsByCoinType[coinType] = Array.from(
      new Set(
        (nonFungibleIdsByCoinType[coinType] || [])
          .concat(tokensRegistry.nonFungibleIdsByCoinType[coinType])
          .concat(userTokensRegistry.nonFungibleIdsByCoinType[coinType])
      )
    )

    // visible ids by coin type
    visibleTokenIdsByCoinType[coinType] = Array.from(
      new Set(
        (visibleTokenIdsByCoinType[coinType] || [])
          .concat(tokensRegistry.visibleTokenIdsByCoinType[coinType])
          .concat(userTokensRegistry.visibleTokenIdsByCoinType[coinType])
      )
    )
    fungibleVisibleTokenIdsByCoinType[coinType] = Array.from(
      new Set(
        (fungibleVisibleTokenIdsByCoinType[coinType] || [])
          .concat(tokensRegistry.fungibleVisibleTokenIdsByCoinType[coinType])
          .concat(
            userTokensRegistry.fungibleVisibleTokenIdsByCoinType[coinType]
          )
      )
    )
    nonFungibleVisibleTokenIdsByCoinType[coinType] = Array.from(
      new Set(
        (nonFungibleVisibleTokenIdsByCoinType[coinType] || [])
          .concat(tokensRegistry.nonFungibleVisibleTokenIdsByCoinType[coinType])
          .concat(
            userTokensRegistry.nonFungibleVisibleTokenIdsByCoinType[coinType]
          )
      )
    )

    // hidden ids by coin type
    hiddenTokenIdsByCoinType[coinType] = Array.from(
      new Set(
        (hiddenTokenIdsByCoinType[coinType] || [])
          .concat(tokensRegistry.hiddenTokenIdsByCoinType[coinType])
          .concat(userTokensRegistry.hiddenTokenIdsByCoinType[coinType])
      )
    )
    fungibleHiddenTokenIdsByCoinType[coinType] = Array.from(
      new Set(
        (fungibleHiddenTokenIdsByCoinType[coinType] || [])
          .concat(tokensRegistry.fungibleHiddenTokenIdsByCoinType[coinType])
          .concat(userTokensRegistry.fungibleHiddenTokenIdsByCoinType[coinType])
      )
    )
    nonFungibleHiddenTokenIdsByCoinType[coinType] = Array.from(
      new Set(
        (nonFungibleHiddenTokenIdsByCoinType[coinType] || [])
          .concat(tokensRegistry.nonFungibleVisibleTokenIdsByCoinType[coinType])
          .concat(
            userTokensRegistry.nonFungibleVisibleTokenIdsByCoinType[coinType]
          )
      )
    )
  }

  const idsByChainId: Record<EntityId, EntityId[]> = {}
  const fungibleIdsByChainId: Record<EntityId, EntityId[]> = {}
  const nonFungibleIdsByChainId: Record<EntityId, EntityId[]> = {}
  const visibleTokenIdsByChainId: Record<string, string[]> = {}
  const hiddenTokenIdsByChainId: Record<string, string[]> = {}
  const nonFungibleVisibleTokenIdsByChainId: Record<string, string[]> = {}
  const nonFungibleHiddenTokenIdsByChainId: Record<string, string[]> = {}
  const fungibleVisibleTokenIdsByChainId: Record<string, string[]> = {}
  const fungibleHiddenTokenIdsByChainId: Record<string, string[]> = {}
  for (const chainId of chainIds) {
    // ids by chain
    idsByChainId[chainId] = Array.from(
      new Set(
        (idsByChainId[chainId] || [])
          .concat(tokensRegistry.idsByChainId[chainId])
          .concat(userTokensRegistry.idsByChainId[chainId])
      )
    )
    fungibleIdsByChainId[chainId] = Array.from(
      new Set(
        (fungibleIdsByChainId[chainId] || [])
          .concat(tokensRegistry.fungibleIdsByChainId[chainId])
          .concat(userTokensRegistry.fungibleIdsByChainId[chainId])
      )
    )
    nonFungibleIdsByChainId[chainId] = Array.from(
      new Set(
        (nonFungibleIdsByChainId[chainId] || [])
          .concat(tokensRegistry.nonFungibleIdsByChainId[chainId])
          .concat(userTokensRegistry.nonFungibleIdsByChainId[chainId])
      )
    )

    // visible ids by chain
    visibleTokenIdsByChainId[chainId] = Array.from(
      new Set(
        (visibleTokenIdsByChainId[chainId] || [])
          .concat(tokensRegistry.visibleTokenIdsByChainId[chainId])
          .concat(userTokensRegistry.visibleTokenIdsByChainId[chainId])
      )
    )
    nonFungibleVisibleTokenIdsByChainId[chainId] = Array.from(
      new Set(
        (nonFungibleVisibleTokenIdsByChainId[chainId] || [])
          .concat(tokensRegistry.nonFungibleVisibleTokenIdsByChainId[chainId])
          .concat(
            userTokensRegistry.nonFungibleVisibleTokenIdsByChainId[chainId]
          )
      )
    )
    fungibleVisibleTokenIdsByChainId[chainId] = Array.from(
      new Set(
        (fungibleVisibleTokenIdsByChainId[chainId] || [])
          .concat(tokensRegistry.fungibleVisibleTokenIdsByChainId[chainId])
          .concat(userTokensRegistry.fungibleVisibleTokenIdsByChainId[chainId])
      )
    )

    // hidden ids by chain
    hiddenTokenIdsByChainId[chainId] = Array.from(
      new Set(
        (hiddenTokenIdsByChainId[chainId] || [])
          .concat(tokensRegistry.hiddenTokenIdsByChainId[chainId])
          .concat(userTokensRegistry.hiddenTokenIdsByChainId[chainId])
      )
    )
    nonFungibleHiddenTokenIdsByChainId[chainId] = Array.from(
      new Set(
        (nonFungibleHiddenTokenIdsByChainId[chainId] || [])
          .concat(tokensRegistry.nonFungibleHiddenTokenIdsByChainId[chainId])
          .concat(
            userTokensRegistry.nonFungibleHiddenTokenIdsByChainId[chainId]
          )
      )
    )
    fungibleHiddenTokenIdsByChainId[chainId] = Array.from(
      new Set(
        (fungibleHiddenTokenIdsByChainId[chainId] || [])
          .concat(tokensRegistry.fungibleHiddenTokenIdsByChainId[chainId])
          .concat(userTokensRegistry.fungibleHiddenTokenIdsByChainId[chainId])
      )
    )
  }

  // all visible ids
  const visibleTokenIds = Array.from(
    new Set(
      tokensRegistry.visibleTokenIds.concat(userTokensRegistry.visibleTokenIds)
    )
  )
  const fungibleVisibleTokenIds = Array.from(
    new Set(
      tokensRegistry.fungibleVisibleTokenIds.concat(
        userTokensRegistry.fungibleVisibleTokenIds
      )
    )
  )
  const nonFungibleVisibleTokenIds = Array.from(
    new Set(
      tokensRegistry.nonFungibleVisibleTokenIds.concat(
        userTokensRegistry.nonFungibleVisibleTokenIds
      )
    )
  )
  // all hidden ids
  const hiddenTokenIds = Array.from(
    new Set(
      tokensRegistry.hiddenTokenIds.concat(userTokensRegistry.hiddenTokenIds)
    )
  )
  const fungibleHiddenTokenIds = Array.from(
    new Set(
      tokensRegistry.fungibleHiddenTokenIds.concat(
        userTokensRegistry.fungibleHiddenTokenIds
      )
    )
  )
  const nonFungibleHiddenTokenIds = Array.from(
    new Set(
      tokensRegistry.nonFungibleVisibleTokenIds.concat(
        userTokensRegistry.nonFungibleVisibleTokenIds
      )
    )
  )

  // all ids
  const nonFungibleTokenIds = Array.from(
    new Set(
      tokensRegistry.nonFungibleTokenIds.concat(
        userTokensRegistry.nonFungibleTokenIds
      )
    )
  )
  const fungibleTokenIds = Array.from(
    new Set(
      tokensRegistry.fungibleTokenIds.concat(
        userTokensRegistry.fungibleTokenIds
      )
    )
  )

  const initialState: BlockchainTokenEntityAdaptorState = {
    // use the tokens registry state to reduce amount of additions
    ...tokensRegistry,

    // unmodified user registry ids
    deletedTokenIds: userTokensRegistry.deletedTokenIds,
    spamTokenIds: userTokensRegistry.spamTokenIds,
    nonSpamTokenIds: userTokensRegistry.nonSpamTokenIds,

    // new combined grouping Ids
    visibleTokenIds,
    hiddenTokenIds,
    idsByChainId,
    visibleTokenIdsByChainId,
    hiddenTokenIdsByChainId,
    idsByCoinType,
    visibleTokenIdsByCoinType,
    hiddenTokenIdsByCoinType,
    fungibleTokenIds,
    fungibleIdsByChainId,
    fungibleIdsByCoinType,
    fungibleVisibleTokenIds,
    fungibleHiddenTokenIds,
    fungibleVisibleTokenIdsByChainId,
    fungibleHiddenTokenIdsByChainId,
    fungibleVisibleTokenIdsByCoinType,
    fungibleHiddenTokenIdsByCoinType,
    nonFungibleTokenIds,
    nonFungibleIdsByChainId,
    nonFungibleIdsByCoinType,
    nonFungibleVisibleTokenIds,
    nonFungibleHiddenTokenIds,
    nonFungibleVisibleTokenIdsByChainId,
    nonFungibleHiddenTokenIdsByChainId,
    nonFungibleVisibleTokenIdsByCoinType,
    nonFungibleHiddenTokenIdsByCoinType
  }

  // add user tokens to known tokens registry
  // if entity id is duplicated,
  // replace existing registry entity info with user token info
  return blockchainTokenEntityAdaptor.setMany(
    initialState,
    getEntitiesListFromEntityState(userTokensRegistry)
  )
}

// Tokens Registry Selectors From Root
const selectTokensRegistry = (state: WalletApiSliceStateFromRoot) => {
  return (
    walletApi.endpoints.getTokensRegistry.select()(state)?.data ??
    blockchainTokenEntityAdaptorInitialState
  )
}

export const {
  selectAll: selectAllBlockchainTokens,
  selectById: selectBlockchainTokenById,
  selectEntities: selectBlockchainTokenEntities,
  selectIds: selectBlockchainTokenIds,
  selectTotal: selectTotalBlockchainTokens
} = blockchainTokenEntityAdaptor.getSelectors(selectTokensRegistry)

export const makeSelectAllBlockchainTokenIdsForChainId = () => {
  return createDraftSafeSelector(
    [selectTokensRegistry, (_, chainId: EntityId) => chainId],
    (registry, chainId) => registry.idsByChainId[chainId]
  )
}

export const makeSelectAllBlockchainTokensForChain = () => {
  return createDraftSafeSelector(
    [selectTokensRegistry, (_, chainId: EntityId) => chainId],
    (registry, chainId) =>
      getEntitiesListFromEntityState(registry, registry.idsByChainId[chainId])
  )
}

// From Query Results
const selectTokensRegistryFromQueryResult = (result: {
  data?: BlockchainTokenEntityAdaptorState
}) => {
  return result.data ?? blockchainTokenEntityAdaptorInitialState
}

export const {
  selectAll: selectAllBlockchainTokensFromQueryResult,
  selectById: selectBlockchainTokenByIdFromQueryResult,
  selectEntities: selectBlockchainTokenEntitiesFromQueryResult,
  selectIds: selectBlockchainTokenIdsFromQueryResult,
  selectTotal: selectTotalBlockchainTokensFromQueryResult
} = blockchainTokenEntityAdaptor.getSelectors(
  selectTokensRegistryFromQueryResult
)

export const makeSelectAllBlockchainTokenIdsForChainIdFromQueryResult = () => {
  return createDraftSafeSelector(
    [selectTokensRegistryFromQueryResult, (_, chainId: EntityId) => chainId],
    (registry, chainId) => registry.idsByChainId[chainId]
  )
}

export const makeSelectAllBlockchainTokensForChainFromQueryResult = () => {
  return createDraftSafeSelector(
    [selectTokensRegistryFromQueryResult, (_, chainId: EntityId) => chainId],
    (registry, chainId) =>
      getEntitiesListFromEntityState(registry, registry.idsByChainId[chainId])
  )
}

// User Assets Registry Selectors from Root State
const selectUserAssetRegistry = (state: WalletApiSliceStateFromRoot) => {
  return (
    walletApi.endpoints.getUserTokensRegistry.select()(state)?.data ??
    blockchainTokenEntityAdaptorInitialState
  )
}

export const {
  selectAll: selectAllUserAssets,
  selectById: selectUserAssetById,
  selectEntities: selectUserAssetEntities,
  selectIds: selectUserAssetIds,
  selectTotal: selectTotalUserAssets
} = blockchainTokenEntityAdaptor.getSelectors(selectUserAssetRegistry)

export const makeSelectUserAssetIdsForChainId = () => {
  return createDraftSafeSelector(
    [selectUserAssetRegistry, (_, chainId: EntityId) => chainId],
    (registry, chainId) => registry.idsByChainId[chainId]
  )
}

export const makeSelectAllUserAssetsForChain = () => {
  return createDraftSafeSelector(
    [selectUserAssetRegistry, (_, chainId: EntityId) => chainId],
    (registry, chainId) =>
      getEntitiesListFromEntityState(registry, registry.idsByChainId[chainId])
  )
}

// User Assets Registry Selectors from Query Result

export const {
  selectAll: selectAllUserAssetsFromQueryResult,
  selectById: selectUserAssetByIdFromQueryResult,
  selectEntities: selectUserAssetEntitiesFromQueryResult,
  selectIds: selectUserAssetIdsFromQueryResult,
  selectTotal: selectTotalUserAssetsFromQueryResult
} = blockchainTokenEntityAdaptor.getSelectors(
  selectTokensRegistryFromQueryResult
)

export const makeSelectUserAssetIdsForChainIdFromQueryResult = () => {
  return createDraftSafeSelector(
    [selectTokensRegistryFromQueryResult, (_, chainId: EntityId) => chainId],
    (registry, chainId) => registry.idsByChainId[chainId]
  )
}

export const makeSelectAllUserAssetsForChainFromQueryResult = () =>
  createDraftSafeSelector(
    [selectTokensRegistryFromQueryResult, (_, chainId: EntityId) => chainId],
    (registry, chainId) =>
      getEntitiesListFromEntityState(registry, registry.idsByChainId[chainId])
  )

// combined tokens list
export const selectCombinedTokensList = createDraftSafeSelector(
  // inputs
  [
    (knownTokens: BraveWallet.BlockchainToken[]) => knownTokens,
    (_, userTokens: BraveWallet.BlockchainToken[]) => userTokens
  ],
  // output
  (knownTokensList, userTokensList) => {
    const filteredKnownTokens = knownTokensList.filter(
      (token) =>
        !userTokensList.some(
          (userToken) =>
            userToken.contractAddress === token.contractAddress &&
            userToken.chainId === token.chainId &&
            userToken.tokenId === token.tokenId
        )
    )
    return userTokensList.concat(filteredKnownTokens)
  }
)

// combined tokens registry
export const selectCombinedTokensRegistry = createDraftSafeSelector(
  // inputs
  [
    (knownTokens: BlockchainTokenEntityAdaptorState) => knownTokens,
    (_, userTokens: BlockchainTokenEntityAdaptorState) => userTokens
  ],
  // output
  (knownTokens, userTokens) => {
    return combineTokenRegistries(knownTokens, userTokens)
  }
)

/**
 * Used to select visible only tokens from useGetUserTokensRegistryQuery
 */
export const selectAllVisibleUserAssetsFromQueryResult =
  createDraftSafeSelector([selectTokensRegistryFromQueryResult], (assets) =>
    getEntitiesListFromEntityState(assets, assets.visibleTokenIds)
  )

/**
 * Used to select only hidden NFTs from useGetUserTokensRegistryQuery
 */
export const selectHiddenNftsFromQueryResult = createDraftSafeSelector(
  [selectTokensRegistryFromQueryResult],
  (assets) =>
    getEntitiesListFromEntityState(assets, assets.hiddenTokenIds).filter(
      (t) => t.isErc1155 || t.isErc721 || t.isNft
    )
)
