// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  createDraftSafeSelector,
  createEntityAdapter,
  EntityAdapter,
  EntityState,
} from '@reduxjs/toolkit'
import { BraveWallet } from '../../../constants/types'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'

export const getNetworkId = ({ chainId }: { chainId: string }): string =>
  chainId

export const networkEntityAdapter: EntityAdapter<BraveWallet.NetworkInfo> =
  createEntityAdapter<BraveWallet.NetworkInfo>({
    selectId: getNetworkId,
  })

export type NetworksRegistry = EntityState<BraveWallet.NetworkInfo> & {
  hiddenIds: string[]
  offRampChainIds: string[]
  ankrChainIds: string[]
  swapChainIds: string[]
}

export const emptyNetworksRegistry: NetworksRegistry = {
  ...networkEntityAdapter.getInitialState(),
  hiddenIds: [],
  offRampChainIds: [],
  ankrChainIds: [],
  swapChainIds: [],
}

const selectNetworksRegistryFromQueryResult = (
  networksRegistry: NetworksRegistry | undefined,
) => {
  return networksRegistry ?? emptyNetworksRegistry
}

export const networkSelectors = {
  ...networkEntityAdapter.getSelectors(selectNetworksRegistryFromQueryResult),
  selectOffRampNetworks: createDraftSafeSelector(
    [selectNetworksRegistryFromQueryResult],
    (registry) =>
      getEntitiesListFromEntityState(registry, registry.offRampChainIds),
  ),
  selectVisibleNetworks: createDraftSafeSelector(
    [selectNetworksRegistryFromQueryResult],
    (registry) =>
      Object.values(registry.entities).filter(
        (network): network is BraveWallet.NetworkInfo =>
          network !== undefined
          && !registry.hiddenIds.includes(network.chainId),
      ),
  ),
  selectSwapNetworks: createDraftSafeSelector(
    [selectNetworksRegistryFromQueryResult],
    (registry) =>
      getEntitiesListFromEntityState(registry, registry.swapChainIds),
  ),
}
