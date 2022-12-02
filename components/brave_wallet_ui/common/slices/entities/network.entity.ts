// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { createEntityAdapter } from '@reduxjs/toolkit'
import { BraveWallet } from '../../../constants/types'

export const networkEntityAdapter = createEntityAdapter<BraveWallet.NetworkInfo>({
  selectId: ({ chainId }) => chainId
})
export const networkEntityInitalState = networkEntityAdapter.getInitialState()
export type NetworkEntityState = typeof networkEntityInitalState

//
// Selectors (From Query Results)
//
export const selectNetworksRegistryFromQueryResult = (result: {
  data?: NetworkEntityState
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
