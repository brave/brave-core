// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

// Utils
import {
  createMockStore,
  renderHookOptionsWithMockStore,
} from '../../utils/test-utils'

// Hooks
import { useIsDAppVerified } from './use_is_dapp_verified'

// Mocks
import { mockUniswapOriginInfo } from '../../stories/mock-data/mock-origin-info'
import { mockEthMainnet } from '../../stories/mock-data/mock-networks'

// Mock the useGetTopDappsQuery hook
jest.mock('../slices/api.slice', () => ({
  ...jest.requireActual('../slices/api.slice'),
  useGetTopDappsQuery: () => ({
    data: [
      {
        id: 4096,
        name: 'Uniswap V2',
        website: 'https://app.uniswap.org/',
        chains: ['ethereum'],
      },
      {
        id: 112,
        name: 'PancakeSwap V2',
        website: 'https://pancakeswap.finance',
        chains: ['binance-smart-chain'],
      },
    ],
  }),
}))

describe('useIsDAppVerified hook', () => {
  it('should return true when origin matches a dapp website origin.', async () => {
    // setup
    const store = createMockStore(
      {},
      {
        networks: [mockEthMainnet],
      },
    )
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(
      () =>
        useIsDAppVerified({
          ...mockUniswapOriginInfo,
        }),
      renderOptions,
    )

    // wait for query to complete
    await waitFor(() => {
      expect(result.current).toBeDefined()
    })

    expect(result.current.isDAppVerified).toBe(true)
    expect(result.current.dapp?.name).toBe('Uniswap V2')
  })

  it('should return false when origin does not exist.', async () => {
    // setup
    const store = createMockStore(
      {},
      {
        networks: [mockEthMainnet],
      },
    )
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(
      () =>
        useIsDAppVerified({
          originSpec: 'https://unknown-dapp.com',
          eTldPlusOne: 'unknown-dapp.com',
        }),
      renderOptions,
    )

    // wait for query to complete
    await waitFor(() => {
      expect(result.current).toBeDefined()
    })

    expect(result.current.isDAppVerified).toBe(false)
    expect(result.current.dapp).toBeUndefined()
  })

  it('should not treat a domain prefix of a verified website as verified.', async () => {
    // setup
    const store = createMockStore(
      {},
      {
        networks: [mockEthMainnet],
      },
    )
    const renderOptions = renderHookOptionsWithMockStore(store)

    // pancakeswap.fi is a string prefix of pancakeswap.finance, but must not
    // inherit the PancakeSwap V2 verified badge.
    const { result } = renderHook(
      () =>
        useIsDAppVerified({
          originSpec: 'https://pancakeswap.fi',
          eTldPlusOne: 'pancakeswap.fi',
        }),
      renderOptions,
    )

    await waitFor(() => {
      expect(result.current).toBeDefined()
    })

    expect(result.current.isDAppVerified).toBe(false)
    expect(result.current.dapp).toBeUndefined()
  })
})
