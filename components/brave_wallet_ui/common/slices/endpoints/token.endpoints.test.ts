// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook, waitFor } from '@testing-library/react'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore
} from '../../../utils/test-utils'
import { selectAllVisibleUserAssetsFromQueryResult } from '../entities/blockchain-token.entity'
import { getAssetIdKey } from '../../../utils/asset-utils'

// hooks
import {
  useAddUserTokenMutation,
  useGetUserTokensRegistryQuery,
  useRemoveUserTokenMutation
} from '../api.slice'

// mocks
import {
  mockMoonCatNFT,
  mockNewAssetOptions
} from '../../../stories/mock-data/mock-asset-options'

const fetchTokensAndSetupStore = async () => {
  const store = createMockStore(
    {},
    {
      userAssets: mockNewAssetOptions
    }
  )

  const hook = renderHook(
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

  // load
  await waitFor(() => !hook.result.current.isLoading)
  await waitFor(() => hook.result.current.visibleTokens.length)

  const { visibleTokens, error, isLoading } = hook.result.current
  expect(isLoading).toBe(false)
  expect(error).not.toBeDefined()
  expect(visibleTokens.length).toBeTruthy()

  return { hook, store }
}

describe('token endpoints', () => {
  it('it should fetch tokens', async () => {
    const { hook } = await fetchTokensAndSetupStore()

    expect(hook.result.current.visibleTokens).toHaveLength(
      mockNewAssetOptions.length
    )
  })

  it('it should delete tokens', async () => {
    const { hook, store } = await fetchTokensAndSetupStore()

    const { visibleTokens } = hook.result.current

    const { result: mutationHook } = renderHook(
      () => useRemoveUserTokenMutation(),
      renderHookOptionsWithMockStore(store)
    )

    const [removeToken] = mutationHook.current

    const tokenToRemove = visibleTokens.find((t) => t.contractAddress !== '')!
    expect(tokenToRemove).toBeTruthy()

    await act(async () => {
      await removeToken(getAssetIdKey(tokenToRemove)).unwrap()
    })
    act(hook.rerender)

    const { visibleTokens: newTokens } = hook.result.current

    expect(newTokens).toHaveLength(visibleTokens.length - 1)
  })

  it('it should add tokens to the registry', async () => {
    const { hook, store } = await fetchTokensAndSetupStore()
    const visibleTokens = hook.result.current.visibleTokens

    const { result: mutationHook } = renderHook(
      () => useAddUserTokenMutation(),
      renderHookOptionsWithMockStore(store)
    )

    const [addToken] = mutationHook.current

    const tokenToAdd: BraveWallet.BlockchainToken = {
      ...mockMoonCatNFT,
      isNft: false,
      tokenId: '2'
    }
    const tokenToAddId = getAssetIdKey(tokenToAdd)

    // add token
    let res
    await act(async () => {
      res = await addToken(tokenToAdd).unwrap()
    })
    expect(res).toEqual({ id: tokenToAddId })

    // get updated list
    act(hook.rerender)
    const { visibleTokens: newTokens } = hook.result.current

    // check new list
    expect(
      newTokens.find((t) => getAssetIdKey(t) === tokenToAddId)
    ).toBeTruthy()
    expect(newTokens).toHaveLength(visibleTokens.length + 1)
  })
})
