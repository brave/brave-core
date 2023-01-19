// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { createDraftSafeSelector, createEntityAdapter, EntityId } from '@reduxjs/toolkit'
import { BraveWallet } from '../../../constants/types'

export const networkEntityAdapter = createEntityAdapter<BraveWallet.NetworkInfo>({
  selectId: ({ chainId }) => chainId
})

export type NetworkEntityAdaptorState = ReturnType<
  typeof networkEntityAdapter['getInitialState']
> & {
  idsByCoinType: Record<BraveWallet.CoinType, EntityId[]>
}

export const networkEntityInitalState: NetworkEntityAdaptorState = {
  ...networkEntityAdapter.getInitialState(),
  idsByCoinType: {}
}

//
// Selectors (From Query Results)
//
export const selectNetworksRegistryFromQueryResult = (result: {
  data?: NetworkEntityAdaptorState
}) => {
  return result.data ?? networkEntityInitalState
}

export const {
  selectAll: selectAllNetworksFromQueryResult,
  selectById: selectNetworkByIdFromQueryResult,
  selectEntities: selectNetworkEntitiesFromQueryResult,
  selectIds: selectNetworkIdsFromQueryResult,
  selectTotal: selectTotalNetworksFromQueryResult
} = networkEntityAdapter.getSelectors(selectNetworksRegistryFromQueryResult)

export const makeSelectAllChainIdsForCoinTypeFromQueryResult = () => {
  return createDraftSafeSelector(
    [
      selectNetworksRegistryFromQueryResult,
      (_, coinType: BraveWallet.CoinType) => coinType
    ],
    (registry, coinType) => registry.idsByCoinType[coinType]
  )
}
