// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react-hooks'

// utils
import { mockWalletState } from '../../../stories/mock-data/mock-wallet-state'
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'
import { selectAllVisibleUserAssetsFromQueryResult } from '../entities/blockchain-token.entity'

// hooks
import {
  useAddUserTokenMutation,
  useGetUserTokensRegistryQuery,
  useRemoveUserTokenMutation
} from '../api.slice'

const fetchTokensAndSetupStore = async () => {
  const store = createMockStore(
    {},
    {
      userAssets: mockWalletState.userVisibleTokensInfo
    }
  )

  const { result, waitForValueToChange, rerender } = renderHook(
    () =>
      useGetUserTokensRegistryQuery(undefined, {
        selectFromResult: (res) => ({
          isLoading: res.isLoading,
          visibleTokens: selectAllVisibleUserAssetsFromQueryResult(res),
          error: res.error
        })
      }),
    renderHookOptionsWithMockStore(store)
  )

  await waitForValueToChange(() => result.current.isLoading)
  const { visibleTokens, isLoading, error } = result.current

  expect(isLoading).toBe(false)
  expect(error).not.toBeDefined()
  expect(visibleTokens).toHaveLength(
    mockWalletState.userVisibleTokensInfo.length
  )

  return { result, rerender, store }
}

describe('token endpoints', () => {
  it('it should fetch tokens', async () => {
    const { result } = await fetchTokensAndSetupStore()
    const visibleTokens = result.current.visibleTokens
    expect(visibleTokens).toHaveLength(
      mockWalletState.userVisibleTokensInfo.length
    )
  })

  it('it should delete tokens', async () => {
    const { result, store, rerender } = await fetchTokensAndSetupStore()
    const visibleTokens = result.current.visibleTokens

    const { result: mutationHook } = renderHook(
      () => useRemoveUserTokenMutation(),
      renderHookOptionsWithMockStore(store)
    )

    const [removeToken] = mutationHook.current

    await act(async () => {
      await removeToken(visibleTokens[0]).unwrap()
    })
    act(rerender)

    const { visibleTokens: newTokens } = result.current

    expect(newTokens).toHaveLength(visibleTokens.length - 1)
  })

  it('it should add tokens', async () => {
    const { result, store, rerender } = await fetchTokensAndSetupStore()
    const visibleTokens = result.current.visibleTokens

    const { result: mutationHook } = renderHook(
      () => useAddUserTokenMutation(),
      renderHookOptionsWithMockStore(store)
    )

    const [addToken] = mutationHook.current

    await act(async () => {
      await addToken(visibleTokens[0]).unwrap()
    })
    act(rerender)

    const { visibleTokens: newTokens } = result.current

    expect(newTokens).toHaveLength(visibleTokens.length + 1)
  })
})
