// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createEntityAdapter, EntityAdapter, EntityId } from '@reduxjs/toolkit'
import { ParsedTransactionWithoutFiatValues, transactionSortByDateComparer } from '../../../utils/tx-utils'

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
