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
    createMockStore({
      walletStateOverride: mockWalletState,
      pageStateOverride: mockPageState
    })

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

    const {
      wallet: { userVisibleTokensInfo }
    } = store.getState()
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

    const {
      wallet: { userVisibleTokensInfo }
    } = store.getState()
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

    const {
      wallet: { userVisibleTokensInfo }
    } = store.getState()
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
    const customStore = createMockStore({
      walletStateOverride: {
        ...mockWalletState,
        userVisibleTokensInfo: [
          mockCustomToken,
          ...mockWalletState.userVisibleTokensInfo
        ]
      },
      pageStateOverride: mockPageState
    })
    const store = makeStore(customStore)
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    const {
      wallet: { userVisibleTokensInfo }
    } = store.getState()
    await act(
      async () =>
        await result.current.onUpdateVisibleAssets([
          { ...mockCustomToken, visible: false },
          ...userVisibleTokensInfo.slice(1)
        ])
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.setUserAssetVisible({
        token: { ...mockCustomToken, visible: false },
        isVisible: false
      })
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should set custom tokens visibility to true', async () => {
    const customStore = createMockStore({
      walletStateOverride: {
        ...mockWalletState,
        userVisibleTokensInfo: [
          { ...mockCustomToken, visible: false },
          ...mockWalletState.userVisibleTokensInfo
        ]
      },
      pageStateOverride: mockPageState
    })
    const store = makeStore(customStore)
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    await act(
      async () =>
        await result.current.onUpdateVisibleAssets([
          mockCustomToken,
          ...mockWalletState.userVisibleTokensInfo
        ])
    )
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.setUserAssetVisible({
        token: mockCustomToken,
        isVisible: true
      })
    )
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should add token to visible assets list if not found', async () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    await act(
      async () => await result.current.makeTokenVisible(mockCustomToken)
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.addUserAsset({ ...mockCustomToken, logo: '' })
    )
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should not add token to visible list if already there', async () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    const {
      wallet: { userVisibleTokensInfo }
    } = store.getState()

    // useGetUserTokensRegistryQuery call
    expect(store.dispatch).toHaveBeenCalledTimes(1)

    await act(
      async () =>
        await result.current.makeTokenVisible(userVisibleTokensInfo[0])
    )

    // No additional dispatches
    expect(store.dispatch).toHaveBeenCalledTimes(1)
  })

  it('should update visibility of token if not visible already', async () => {
    const store = makeStore(
      createMockStore({
        walletStateOverride: {
          ...mockWalletState,
          userVisibleTokensInfo: [
            ...mockWalletState.userVisibleTokensInfo,
            { ...mockCustomToken, visible: false }
          ]
        },
        pageStateOverride: mockPageState
      })
    )
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    // useGetUserTokensRegistryQuery call
    expect(store.dispatch).toHaveBeenCalledTimes(1)

    await act(
      async () => await result.current.makeTokenVisible(mockCustomToken)
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.setUserAssetVisible({
        token: mockCustomToken,
        isVisible: true
      })
    )
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })
})
