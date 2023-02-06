// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { createDraftSafeSelector, createEntityAdapter, EntityAdapter, EntityId } from '@reduxjs/toolkit'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getAssetIdKey, GetBlockchainTokenIdArg } from '../../../utils/asset-utils'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import { walletApi, WalletApiSliceStateFromRoot } from '../api.slice'

export type BlockchainTokenEntityAdaptor = EntityAdapter<BraveWallet.BlockchainToken> & {
  selectId: (t: GetBlockchainTokenIdArg) => EntityId
}

export const blockchainTokenEntityAdaptor: BlockchainTokenEntityAdaptor = createEntityAdapter<BraveWallet.BlockchainToken>({
  selectId: getAssetIdKey
})
export type BlockchainTokenEntityAdaptorState = ReturnType<BlockchainTokenEntityAdaptor['getInitialState']> & {
  idsByChainId: Record<EntityId, EntityId[]>
  idsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  visibleTokenIds: string[]
  visibleTokenIdsByChainId: Record<string, string[]>
  visibleTokenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
}

export const blockchainTokenEntityAdaptorInitialState: BlockchainTokenEntityAdaptorState = {
  ...blockchainTokenEntityAdaptor.getInitialState(),
  idsByChainId: {},
  idsByCoinType: {},
  visibleTokenIds: [],
  visibleTokenIdsByChainId: {},
  visibleTokenIdsByCoinType: {}
}

export const combineTokenRegistries = (
  tokensRegistry: BlockchainTokenEntityAdaptorState,
  userTokensRegistry: BlockchainTokenEntityAdaptorState
): BlockchainTokenEntityAdaptorState => {
  const idsByChainId: Record<EntityId, EntityId[]> = {
    ...tokensRegistry.idsByChainId
  }
  Object.keys(userTokensRegistry.idsByChainId).forEach((key) => {
    idsByChainId[key] = (idsByChainId[key] || []).concat(
      userTokensRegistry.idsByChainId[key]
    )
  })

  const visibleTokenIdsByChainId: Record<string, string[]> = {
    ...tokensRegistry.visibleTokenIdsByChainId
  }
  Object.keys(userTokensRegistry.visibleTokenIdsByChainId).forEach((key) => {
    visibleTokenIdsByChainId[key] = (
      visibleTokenIdsByChainId[key] || []
    ).concat(userTokensRegistry.visibleTokenIdsByChainId[key])
  })

  const idsByCoinType: Record<BraveWallet.CoinType, EntityId[]> = {
    ...tokensRegistry.idsByCoinType
  }
  Object.keys(userTokensRegistry.idsByCoinType).forEach((key) => {
    idsByCoinType[key] = (idsByCoinType[key] || []).concat(
      userTokensRegistry.idsByCoinType[key]
    )
  })

  const visibleTokenIdsByCoinType: Record<number, EntityId[]> = {
    ...tokensRegistry.visibleTokenIdsByCoinType
  }
  Object.keys(userTokensRegistry.visibleTokenIdsByCoinType).forEach((key) => {
    visibleTokenIdsByCoinType[key] = (
      visibleTokenIdsByCoinType[key] || []
    ).concat(userTokensRegistry.visibleTokenIdsByCoinType[key])
  })

  const visibleTokenIds = [
    ...new Set([
      ...tokensRegistry.visibleTokenIds,
      ...userTokensRegistry.visibleTokenIds
    ])
  ]

  const ids = [...new Set([...tokensRegistry.ids, ...userTokensRegistry.ids])]

  return blockchainTokenEntityAdaptor.setAll(
    {
      ...blockchainTokenEntityAdaptorInitialState,
      entities: {
        ...tokensRegistry.entities,
        ...userTokensRegistry.entities
      },
      ids,
      visibleTokenIds,
      idsByChainId,
      visibleTokenIdsByChainId,
      idsByCoinType,
      visibleTokenIdsByCoinType
    },
    getEntitiesListFromEntityState(tokensRegistry).concat(
      getEntitiesListFromEntityState(userTokensRegistry)
    )
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
    [
      selectTokensRegistry,
      (_, chainId: EntityId) => chainId
    ],
    (registry, chainId) => registry.idsByChainId[chainId]
  )
}

export const makeSelectAllBlockchainTokensForChain = () => {
  return createDraftSafeSelector(
    [
      selectTokensRegistry,
      (_, chainId: EntityId) => chainId
    ],
    (registry, chainId) => getEntitiesListFromEntityState(registry, registry.idsByChainId[chainId])
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
    [
      selectTokensRegistryFromQueryResult,
      (_, chainId: EntityId) => chainId
    ],
    (registry, chainId) => registry.idsByChainId[chainId]
  )
}

export const makeSelectAllBlockchainTokensForChainFromQueryResult = () => {
  return createDraftSafeSelector(
    [
      selectTokensRegistryFromQueryResult,
      (_, chainId: EntityId) => chainId
    ],
    (registry, chainId) => getEntitiesListFromEntityState(registry, registry.idsByChainId[chainId])
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
    [
      selectUserAssetRegistry,
      (_, chainId: EntityId) => chainId
    ],
    (registry, chainId) => registry.idsByChainId[chainId]
  )
}

export const makeSelectAllUserAssetsForChain = () => {
  return createDraftSafeSelector(
    [
      selectUserAssetRegistry,
      (_, chainId: EntityId) => chainId
    ],
    (registry, chainId) => getEntitiesListFromEntityState(registry, registry.idsByChainId[chainId])
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
    [
      selectTokensRegistryFromQueryResult,
      (_, chainId: EntityId) => chainId
    ],
    (registry, chainId) => registry.idsByChainId[chainId]
  )
}

export const makeSelectAllUserAssetsForChainFromQueryResult = () => {
  return createDraftSafeSelector(
    [
      selectTokensRegistryFromQueryResult,
      (_, chainId: EntityId) => chainId
    ],
    (registry, chainId) => getEntitiesListFromEntityState(registry, registry.idsByChainId[chainId])
  )
}
