// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook } from '@testing-library/react-hooks'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'
import {
  deserializeTransaction,
  makeSerializableTransaction
} from '../../../utils/model-serialization-utils'

// hooks
import { useTransactionQuery } from '../api.slice.extra'

// mocks
import { mockAccount } from '../../constants/mocks'
import {
  mockTransactionInfo //
} from '../../../stories/mock-data/mock-transaction-info'

describe('useTransactionQuery', () => {
  it('finds a transaction from the cache', async () => {
    // setup
    const mockTx = deserializeTransaction(mockTransactionInfo)
    const store = createMockStore(
      {},
      {
        accountInfos: [mockAccount],
        transactionInfos: [mockTx]
      }
    )
    const renderOptions = renderHookOptionsWithMockStore(store)
    const hook = renderHook(
      () => useTransactionQuery(mockTransactionInfo.id),
      renderOptions
    )
    const hookInstance2 = renderHook(
      () => useTransactionQuery(mockTransactionInfo.id),
      renderOptions
    )

    // initial state
    expect(hook.result.current.isLoading).toBeDefined()
    expect(hook.result.current.isLoading).toBe(true)
    expect(hook.result.current.transaction).not.toBeDefined()

    // loading
    await hook.waitFor(() => hook.result.all.length > 2)

    // loaded
    expect(hook.result.current.isLoading).toBe(false)
    expect(hook.result.current.transaction?.id).toBeDefined()
    expect(hook.result.current.transaction).toEqual(
      makeSerializableTransaction(mockTx)
    )

    // additional instances should not
    // create more than one transaction in memory
    expect(hookInstance2.result.current.transaction).toBe(
      hook.result.current.transaction
    )
  })
})
