// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'

// hooks
import { useAccountQuery } from '../api.slice.extra'

// mocks
import { mockAccount } from '../../constants/mocks'

describe('useAccountQuery', () => {
  it('finds a transaction from the cache', async () => {
    // setup
    const store = createMockStore(
      {},
      {
        accountInfos: [mockAccount]
      }
    )
    const renderOptions = renderHookOptionsWithMockStore(store)
    const hook = renderHook(
      () => useAccountQuery(mockAccount.accountId),
      renderOptions
    )
    const hookInstance2 = renderHook(
      () => useAccountQuery(mockAccount.accountId),
      renderOptions
    )

    // initial state
    expect(hook.result.current.isLoading).toBeDefined()
    expect(hook.result.current.isLoading).toBe(true)
    expect(hook.result.current.account).not.toBeDefined()

    // loading
    await waitFor(() => !hook.result.current.isLoading)
    await waitFor(() => hook.result.current.account)

    // loaded
    expect(hook.result.current.isLoading).toBe(false)
    expect(hook.result.current.error).not.toBeDefined()
    expect(hook.result.current.account).toBeDefined()
    expect(hook.result.current.account?.address).toEqual(mockAccount.address)

    // additional instances should not
    // create more than one account in memory
    expect(hookInstance2.result.current.account).toBe(
      hook.result.current.account
    )
  })
})
