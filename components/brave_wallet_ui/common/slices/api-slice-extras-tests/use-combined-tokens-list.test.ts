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

// hooks
import { useGetCombinedTokensListQuery } from '../api.slice.extra'

// mocks
import {
  mockAccountAssetOptions,
  mockErc20TokensList
} from '../../../stories/mock-data/mock-asset-options'

describe('useCombinedTokensList', () => {
  it('returns the combo of user assets and known tokens', async () => {
    const store = createMockStore(
      {},
      {
        blockchainTokens: mockErc20TokensList,
        userAssets: mockAccountAssetOptions
      }
    )
    const renderOptions = renderHookOptionsWithMockStore(store)
    const { result, ...hook } = renderHook(
      () => useGetCombinedTokensListQuery(),
      renderOptions
    )

    // initial state
    expect(result.current.data).toBeDefined()
    expect(result.current.isLoading).toBeDefined()
    expect(result.current.isLoading).toBe(true)

    // loading
    await hook.waitFor(() => result.all.length > 3)

    // done loading
    expect(result.current.isLoading).toBe(false)
    expect(result.current.data).toHaveLength(19)
  })
})
