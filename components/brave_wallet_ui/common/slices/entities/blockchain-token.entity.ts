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
  visibleTokenIdsByChainId: Record<string, string[]>
  visibleTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>

  // fungible tokens
  fungibleIdsByChainId: Record<EntityId, EntityId[]>
  fungibleIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  fungibleTokenIds: string[]
  fungibleVisibleTokenIds: string[]
  fungibleHiddenTokenIds: string[]
  fungibleVisibleTokenIdsByChainId: Record<string, string[]>
  fungibleVisibleTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>

  // non-fungible tokens
  nonFungibleIdsByChainId: Record<EntityId, EntityId[]>
  nonFungibleIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  nonFungibleTokenIds: string[]
  nonFungibleVisibleTokenIds: string[]
  nonFungibleHiddenTokenIds: string[]
  nonFungibleVisibleTokenIdsByChainId: Record<string, string[]>
  nonFungibleVisibleTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
}

export const blockchainTokenEntityAdaptorInitialState: //
BlockchainTokenEntityAdaptorState = {
  ...blockchainTokenEntityAdaptor.getInitialState(),
  idsByChainId: {},
  idsByCoinType: {},
  visibleTokenIds: [],
  hiddenTokenIds: [],
  visibleTokenIdsByChainId: {},
  visibleTokenIdsByCoinType: {},

  fungibleIdsByChainId: {},
  fungibleIdsByCoinType: {},
  fungibleTokenIds: [],
  fungibleHiddenTokenIds: [],
  fungibleVisibleTokenIds: [],
  fungibleVisibleTokenIdsByChainId: {},
  fungibleVisibleTokenIdsByCoinType: {},

  nonFungibleIdsByChainId: {},
  nonFungibleIdsByCoinType: {},
  nonFungibleTokenIds: [],
  nonFungibleHiddenTokenIds: [],
  nonFungibleVisibleTokenIds: [],
  nonFungibleVisibleTokenIdsByChainId: {},
  nonFungibleVisibleTokenIdsByCoinType: {}
}

export const combineTokenRegistries = (
  tokensRegistry: BlockchainTokenEntityAdaptorState,
  userTokensRegistry: BlockchainTokenEntityAdaptorState
): BlockchainTokenEntityAdaptorState => {
  // TODO: hidden ids
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
  const fungibleVisibleTokenIdsByCoinType: Record<number, EntityId[]> = {}
  const nonFungibleVisibleTokenIdsByCoinType: Record<number, EntityId[]> = {}
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
  }

  const idsByChainId: Record<EntityId, EntityId[]> = {}
  const fungibleIdsByChainId: Record<EntityId, EntityId[]> = {}
  const nonFungibleIdsByChainId: Record<EntityId, EntityId[]> = {}
  const visibleTokenIdsByChainId: Record<string, string[]> = {}
  const nonFungibleVisibleTokenIdsByChainId: Record<string, string[]> = {}
  const fungibleVisibleTokenIdsByChainId: Record<string, string[]> = {}
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

    // new combined grouping Ids
    visibleTokenIds,
    idsByChainId,
    visibleTokenIdsByChainId,
    idsByCoinType,
    visibleTokenIdsByCoinType,
    fungibleTokenIds,
    fungibleIdsByChainId,
    fungibleIdsByCoinType,
    fungibleVisibleTokenIds,
    fungibleVisibleTokenIdsByChainId,
    fungibleVisibleTokenIdsByCoinType,
    nonFungibleTokenIds,
    nonFungibleIdsByChainId,
    nonFungibleIdsByCoinType,
    nonFungibleVisibleTokenIds,
    nonFungibleVisibleTokenIdsByChainId,
    nonFungibleVisibleTokenIdsByCoinType
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
