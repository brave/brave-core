// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, renderHook } from '@testing-library/react-hooks'

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
  expect(visibleTokens.length).toBeTruthy()

  return { result, rerender, store, waitForValueToChange }
}

describe('token endpoints', () => {
  it('it should fetch tokens', async () => {
    const { result } = await fetchTokensAndSetupStore()
    const visibleTokens = result.current.visibleTokens
    expect(visibleTokens).toHaveLength(mockNewAssetOptions.length)
  })

  it('it should delete tokens', async () => {
    const { result, store, rerender } = await fetchTokensAndSetupStore()
    const visibleTokens = result.current.visibleTokens

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
    act(rerender)

    const { visibleTokens: newTokens } = result.current

    expect(newTokens).toHaveLength(visibleTokens.length - 1)
  })

  it('it should add tokens to the registry', async () => {
    const { result, store, rerender } = await fetchTokensAndSetupStore()
    const visibleTokens = result.current.visibleTokens

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
    act(rerender)
    const { visibleTokens: newTokens } = result.current

    // check new list
    expect(
      newTokens.find((t) => getAssetIdKey(t) === tokenToAddId)
    ).toBeTruthy()
    expect(newTokens).toHaveLength(visibleTokens.length + 1)
  })
})
