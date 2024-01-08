// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { renderHook, act } from '@testing-library/react-hooks'

// Redux
import { Provider } from 'react-redux'

// Constants
import { BraveWallet } from '../../constants/types'

// Actions
import * as WalletActions from '../actions/wallet_actions'

// Hooks
import { TextEncoder, TextDecoder } from 'util'
global.TextDecoder = TextDecoder as any
global.TextEncoder = TextEncoder
import useAssetManagement from './assets-management'

// Mocks
import * as MockedLib from '../async/__mocks__/lib'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { mockPageState } from '../../stories/mock-data/mock-page-state'
import { LibContext } from '../context/lib.context'
import { createMockStore } from '../../utils/test-utils'
import {
  selectAllVisibleUserAssetsFromQueryResult //
} from '../slices/entities/blockchain-token.entity'
import { useGetUserTokensRegistryQuery } from '../slices/api.slice'

const mockCustomToken = {
  contractAddress: 'customTokenContractAddress',
  name: 'Custom Token',
  symbol: 'CT',
  logo: '',
  isErc20: true,
  isErc721: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  chainId: '0x1'
} as BraveWallet.BlockchainToken

const makeStore = (customStore?: ReturnType<typeof createMockStore>) => {
  const store =
    customStore ||
    createMockStore(
      {
        walletStateOverride: mockWalletState,
        pageStateOverride: mockPageState
      },
      { userAssets: mockWalletState.userVisibleTokensInfo }
    )

  store.dispatch = jest.fn(store.dispatch)
  return store
}

function renderHookOptionsWithCustomStore(store: any) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) => (
      <Provider store={store}>
        <LibContext.Provider value={MockedLib as any}>
          {children}
        </LibContext.Provider>
      </Provider>
    )
  }
}

describe('useAssetManagement hook', () => {
  it('should add an asset', async () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    // useGetUserTokensRegistryQuery
    const { result: tokensResult, rerender: rerenderTokens } = renderHook(
      () =>
        useGetUserTokensRegistryQuery(undefined, {
          selectFromResult: (res) => ({
            userVisibleTokensInfo:
              selectAllVisibleUserAssetsFromQueryResult(res)
          })
        }),
      renderHookOptions
    )
    await act(async () => rerenderTokens())
    expect(store.dispatch).toHaveBeenCalledTimes(2)
    const { userVisibleTokensInfo } = tokensResult.current

    // update
    await act(
      async () =>
        await result.current.onUpdateVisibleAssets([
          ...userVisibleTokensInfo,
          mockCustomToken
        ])
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.addUserAsset({ ...mockCustomToken, logo: '' })
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should remove an asset', async () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    // useGetUserTokensRegistryQuery
    const { result: tokensResult, rerender: rerenderTokens } = renderHook(
      () =>
        useGetUserTokensRegistryQuery(undefined, {
          selectFromResult: (res) => ({
            userVisibleTokensInfo:
              selectAllVisibleUserAssetsFromQueryResult(res)
          })
        }),
      renderHookOptions
    )
    await act(async () => rerenderTokens())
    expect(store.dispatch).toHaveBeenCalledTimes(2)
    const { userVisibleTokensInfo } = tokensResult.current

    // remove one item from the list
    const newList = userVisibleTokensInfo.slice(1)
    expect(newList.length).toEqual(userVisibleTokensInfo.length - 1)
    expect(newList[0].visible).toBe(true)
    await act(async () =>
      result.current.onUpdateVisibleAssets(userVisibleTokensInfo.slice(1))
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.removeUserAsset(userVisibleTokensInfo[0])
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should remove and add an asset', async () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    // useGetUserTokensRegistryQuery
    const { result: tokensResult, rerender: rerenderTokens } = renderHook(
      () =>
        useGetUserTokensRegistryQuery(undefined, {
          selectFromResult: (res) => ({
            userVisibleTokensInfo:
              selectAllVisibleUserAssetsFromQueryResult(res)
          })
        }),
      renderHookOptions
    )
    await act(async () => rerenderTokens())
    expect(store.dispatch).toHaveBeenCalledTimes(2)
    const { userVisibleTokensInfo } = tokensResult.current

    // update assets
    await act(
      async () =>
        await result.current.onUpdateVisibleAssets([
          ...userVisibleTokensInfo.slice(1),
          mockCustomToken
        ])
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.addUserAsset({ ...mockCustomToken, logo: '' })
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.removeUserAsset(userVisibleTokensInfo[0])
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should set custom tokens visibility to false', async () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    // useGetUserTokensRegistryQuery
    const { result: tokensResult, rerender: rerenderTokens } = renderHook(
      () =>
        useGetUserTokensRegistryQuery(undefined, {
          selectFromResult: (res) => ({
            userVisibleTokensInfo:
              selectAllVisibleUserAssetsFromQueryResult(res)
          })
        }),
      renderHookOptions
    )
    await act(async () => rerenderTokens())
    expect(store.dispatch).toHaveBeenCalledTimes(2)
    const { userVisibleTokensInfo } = tokensResult.current
    expect(userVisibleTokensInfo).toHaveLength(5)

    // clone assets in list so we can modify them
    const newList = userVisibleTokensInfo.map((t) => ({ ...t }))
    // change first asset's visibility
    const changedAsset = newList[0]
    expect(changedAsset.visible).toBe(true)
    changedAsset.visible = !changedAsset.visible
    expect(changedAsset.visible).toBe(false)

    // update assets in store
    await act(async () => await result.current.onUpdateVisibleAssets(newList))

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.setUserAssetVisible({
        token: changedAsset,
        isVisible: false
      })
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should set custom tokens visibility to true', async () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    // useGetUserTokensRegistryQuery
    const { result: tokensResult, rerender: rerenderTokens } = renderHook(
      () =>
        useGetUserTokensRegistryQuery(undefined, {
          selectFromResult: (res) => ({
            userVisibleTokensInfo:
              selectAllVisibleUserAssetsFromQueryResult(res)
          })
        }),
      renderHookOptions
    )
    await act(async () => rerenderTokens())
    expect(store.dispatch).toHaveBeenCalledTimes(2)
    const { userVisibleTokensInfo } = tokensResult.current
    expect(userVisibleTokensInfo).toHaveLength(5)

    // clone assets in list so we can modify them
    const newList = userVisibleTokensInfo.map((t) => ({ ...t }))
    // change first asset's visibility
    const changedAsset = newList[0]
    expect(changedAsset.visible).toBe(true)
    changedAsset.visible = !changedAsset.visible
    expect(changedAsset.visible).toBe(false)

    // update
    await act(async () => await result.current.onUpdateVisibleAssets(newList))
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.setUserAssetVisible({
        token: changedAsset,
        isVisible: changedAsset.visible
      })
    )
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })
})
