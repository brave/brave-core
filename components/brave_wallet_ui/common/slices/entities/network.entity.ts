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
import { BraveWallet } from '../../../constants/types'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'

export const getNetworkId = ({ chainId, coin }: {
  chainId: string
  coin: BraveWallet.CoinType
}): string =>
chainId === BraveWallet.LOCALHOST_CHAIN_ID
  ? `${chainId}-${coin}`
  : chainId

export type NetworkEntityAdaptor = EntityAdapter<BraveWallet.NetworkInfo> & {
  selectId: (network: {
    chainId: string
    coin: BraveWallet.CoinType
  }) => EntityId
}

export const networkEntityAdapter: NetworkEntityAdaptor =
  createEntityAdapter<BraveWallet.NetworkInfo>({
    selectId: getNetworkId
  })

export type NetworksRegistry = ReturnType<
  (typeof networkEntityAdapter)['getInitialState']
> & {
  hiddenIds: string[]
  hiddenIdsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  idsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
  mainnetIds: string[]
  testnetIds: string[]
  onRampIds: string[]
  offRampIds: string[]
  visibleIds: string[]
}

export const emptyNetworksRegistry: NetworksRegistry = {
  ...networkEntityAdapter.getInitialState(),
  hiddenIds: [],
  hiddenIdsByCoinType: {},
  idsByCoinType: {},
  mainnetIds: [],
  testnetIds: [],
  onRampIds: [],
  offRampIds: [],
  visibleIds: []
}

//
// Selectors (From Query Results)
//
export const selectNetworksRegistryFromQueryResult = (result: {
  data?: NetworksRegistry
}) => {
  return result.data ?? emptyNetworksRegistry
}

export const {
  selectAll: selectAllNetworksFromQueryResult,
  selectById: selectNetworkByIdFromQueryResult,
  selectEntities: selectNetworkEntitiesFromQueryResult,
  selectIds: selectNetworkIdsFromQueryResult,
  selectTotal: selectTotalNetworksFromQueryResult
} = networkEntityAdapter.getSelectors(selectNetworksRegistryFromQueryResult)

export const selectMainnetNetworksFromQueryResult = createDraftSafeSelector(
  // inputs
  [selectNetworksRegistryFromQueryResult],
  // output
  (registry) => getEntitiesListFromEntityState(registry, registry.mainnetIds)
)

export const selectOnRampNetworksFromQueryResult = createDraftSafeSelector(
  // inputs
  [selectNetworksRegistryFromQueryResult],
  // output
  (registry) => getEntitiesListFromEntityState(registry, registry.onRampIds)
)

export const selectOffRampNetworksFromQueryResult = createDraftSafeSelector(
  // inputs
  [selectNetworksRegistryFromQueryResult],
  // output
  (registry) => getEntitiesListFromEntityState(registry, registry.offRampIds)
)

export const selectVisibleNetworksFromQueryResult = createDraftSafeSelector(
  // inputs
  [selectNetworksRegistryFromQueryResult],
  // output
  (registry) => getEntitiesListFromEntityState(registry, registry.visibleIds)
)
