// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

// Utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../utils/test-utils'

// Hooks
import { useIsDAppVerified } from './use_is_dapp_verified'

// Mocks
import { mockUniswapOriginInfo } from '../../stories/mock-data/mock-origin-info'
import { mockEthMainnet } from '../../stories/mock-data/mock-networks'

describe('useIsDAppVerified hook', () => {
  it('should return true when origin exists.', async () => {
    // setup
    const store = createMockStore(
      {},
      {
        networks: [mockEthMainnet]
      }
    )
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(
      () =>
        useIsDAppVerified({
          ...mockUniswapOriginInfo
        }),
      renderOptions
    )

    // wait for query to complete
    await waitFor(() => {
      expect(result.current).toBeDefined()
    })

    expect(result.current).toBe(true)
  })

  it('should return false when origin does not exist.', async () => {
    // setup
    const store = createMockStore(
      {},
      {
        networks: [mockEthMainnet]
      }
    )
    const renderOptions = renderHookOptionsWithMockStore(store)

    const { result } = renderHook(
      () =>
        useIsDAppVerified({
          originSpec: 'https://unknown-dapp.com',
          eTldPlusOne: 'unknown-dapp.com'
        }),
      renderOptions
    )

    // wait for query to complete
    await waitFor(() => {
      expect(result.current).toBeDefined()
    })

    expect(result.current).toBe(false)
  })
})
