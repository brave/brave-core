// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  createEntityAdapter,
  EntityAdapter,
  EntityId
} from '@reduxjs/toolkit'
import { BraveWallet } from '../../../constants/types'

export type NetworkEntityAdaptor = EntityAdapter<BraveWallet.NetworkInfo> & {
  selectId: (network: {
    chainId: string
    coin: BraveWallet.CoinType
  }) => EntityId
}

export const networkEntityAdapter: NetworkEntityAdaptor =
  createEntityAdapter<BraveWallet.NetworkInfo>({
    selectId: ({ chainId, coin }): string =>
      chainId === BraveWallet.LOCALHOST_CHAIN_ID
        ? `${chainId}-${coin}`
        : chainId
  })

export type NetworksRegistry = ReturnType<
  typeof networkEntityAdapter['getInitialState']
>

export const emptyNetworksRegistry: NetworksRegistry =
  networkEntityAdapter.getInitialState()

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
