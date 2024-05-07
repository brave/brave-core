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
import { useGetIsRegistryTokenQuery } from '../api.slice.extra'

// mocks
import { mockSplBat } from '../../../stories/mock-data/mock-asset-options'

describe('useGetIsRegistryTokenQuery', () => {
  it('correctly detects if a token is in the known tokens registry', async () => {
    // setup
    const store = createMockStore({})
    const renderOptions = renderHookOptionsWithMockStore(store)
    const hook = renderHook(
      () =>
        useGetIsRegistryTokenQuery({
          address: mockSplBat.contractAddress,
          chainId: mockSplBat.chainId
        }),
      renderOptions
    )

    // initial state
    expect(hook.result.current.isLoading).toBeDefined()
    expect(hook.result.current.isLoading).toBe(true)
    expect(hook.result.current.isVerified).not.toBeDefined()

    // loading
    await waitFor(() => !hook.result.current.isLoading)
    await waitFor(() => hook.result.current.isVerified !== undefined)

    // loaded
    expect(hook.result.current.isLoading).toBe(false)
    expect(hook.result.current.isVerified).toEqual(true)
  })

  it('correctly detects if a token is not in the known tokens registry', async () => {
    // setup
    const store = createMockStore({})
    const renderOptions = renderHookOptionsWithMockStore(store)
    const hook = renderHook(
      () =>
        useGetIsRegistryTokenQuery({
          address: 'unknown',
          chainId: mockSplBat.chainId
        }),
      renderOptions
    )

    // initial state
    expect(hook.result.current.isLoading).toBeDefined()
    expect(hook.result.current.isLoading).toBe(true)
    expect(hook.result.current.isVerified).not.toBeDefined()

    // loading
    await waitFor(() => !hook.result.current.isLoading)
    await waitFor(() => hook.result.current.isVerified !== undefined)

    // loaded
    expect(hook.result.current.isLoading).toBe(false)
    expect(hook.result.current.isVerified).toEqual(false)
  })
})
