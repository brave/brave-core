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
import { walletApi, WalletApiSliceStateFromRoot } from '../api.slice'
import {
  EntityByIdFromRegistryResultSelectorFactory,
  makeSelectEntityByIdFromRegistryQuery
} from './entity.selectors'
import { entityIdFromAccountId } from '../../../utils/account-utils'

export type AccountInfoEntity = BraveWallet.AccountInfo

export type AccountInfoEntityAdaptor = EntityAdapter<AccountInfoEntity> & {
  selectIdByAddress: (address: string) => EntityId
  selectIdByAccountId: (
    accountId: Pick<BraveWallet.AccountId, 'address' | 'uniqueKey'>
  ) => EntityId
}

export const accountInfoEntityAdaptor: AccountInfoEntityAdaptor = {
  ...createEntityAdapter<AccountInfoEntity>({
    selectId: (model: Pick<BraveWallet.AccountInfo, 'accountId'>): EntityId => {
      return entityIdFromAccountId(model.accountId)
    }
  }),
  selectIdByAddress: (address: string) => {
    return entityIdFromAccountId({ address, uniqueKey: '' })
  },
  selectIdByAccountId: entityIdFromAccountId
}

export const accountInfoEntityAdaptorInitialState = accountInfoEntityAdaptor.getInitialState()
export type AccountInfoEntityState = typeof accountInfoEntityAdaptorInitialState

// Selectors from Root State
export const {
  selectAll: selectAllAccountInfos,
  selectById: selectAccountInfoById,
  selectEntities: selectAccountInfoEntities,
  selectIds: selectAccountInfoIds,
  selectTotal: selectTotalAccountInfos
} = accountInfoEntityAdaptor.getSelectors((state: WalletApiSliceStateFromRoot) => {
  return (
    walletApi.endpoints.getAccountInfosRegistry.select()(state)?.data ??
    accountInfoEntityAdaptor.getInitialState()
  )
})

// Selectors from Query Results
export const selectAccountInfosRegistryFromQuery = (result: {
  data?: AccountInfoEntityState
}): AccountInfoEntityState => {
  return result.data ?? accountInfoEntityAdaptorInitialState
}

export const {
  selectAll: selectAllAccountInfosFromQuery,
  selectById: selectAccountInfoByIdFromQuery,
  selectEntities: selectAccountInfoEntitiesFromQuery,
  selectIds: selectAccountInfoIdsFromQuery,
  selectTotal: selectTotalAccountInfosFromQuery
} = accountInfoEntityAdaptor.getSelectors(
  selectAccountInfosRegistryFromQuery
)

type MakeSelectNetworkByIdFromQuery =
  EntityByIdFromRegistryResultSelectorFactory<BraveWallet.NetworkInfo>

export const makeSelectNetworkByIdFromQuery: MakeSelectNetworkByIdFromQuery =
  makeSelectEntityByIdFromRegistryQuery
