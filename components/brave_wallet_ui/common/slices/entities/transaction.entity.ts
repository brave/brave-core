// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createEntityAdapter, EntityAdapter, EntityId } from '@reduxjs/toolkit'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import {
  ParsedTransactionWithoutFiatValues,
  transactionSortByDateComparer
} from '../../../utils/tx-utils'

export type TransactionEntity = ParsedTransactionWithoutFiatValues
export type TransactionEntityAdaptor = EntityAdapter<TransactionEntity>

export const transactionEntityAdapter: TransactionEntityAdaptor =
  createEntityAdapter<TransactionEntity>({
    sortComparer: transactionSortByDateComparer('descending')
  })

export type TransactionEntityState = ReturnType<
  typeof transactionEntityAdapter['getInitialState']
> & {
  idsByChainId: Record<EntityId, EntityId[]>
  pendingIds: EntityId[]
  pendingIdsByChainId: Record<EntityId, EntityId[]>
}

export const transactionEntityInitialState: TransactionEntityState = {
  ...transactionEntityAdapter.getInitialState(),
  idsByChainId: {},
  pendingIds: [],
  pendingIdsByChainId: {}
}

export const combineTransactionRegistries = (
  initialRegistry: TransactionEntityState,
  registriesToAdd: TransactionEntityState[]
): TransactionEntityState => {
  const idsByChainId = {}
  const pendingIdsByChainId = {}
  const pendingIds: EntityId[] = []
  const combinedRegistry: TransactionEntityState = registriesToAdd.reduce(
    (combinedReg, registry) => {
      for (const key in registry.idsByChainId) {
        idsByChainId[key] = Array.from(
          new Set((idsByChainId[key] || []).concat(registry.idsByChainId[key]))
        )
      }
      for (const key in registry.pendingIdsByChainId) {
        pendingIdsByChainId[key] = Array.from(
          new Set(
            (pendingIdsByChainId[key] || []).concat(
              registry.pendingIdsByChainId[key]
            )
          )
        )
      }
      pendingIds.push(...registry.pendingIds)
      return transactionEntityAdapter.upsertMany(
        combinedReg,
        getEntitiesListFromEntityState(registry)
      )
    },
    initialRegistry
  )
  return {
    ...combinedRegistry,
    idsByChainId,
    pendingIds,
    pendingIdsByChainId
  }
}
