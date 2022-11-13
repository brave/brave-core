// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { renderHook, act } from '@testing-library/react-hooks'

// Redux
import { Provider } from 'react-redux'
import { combineReducers, createStore } from 'redux'
import { createWalletReducer } from '../slices/wallet.slice'
import { createPageReducer } from '../../page/reducers/page_reducer'

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

const makeStore = (customStore?: any) => {
  const store = customStore || createStore(combineReducers({
    wallet: createWalletReducer(mockWalletState),
    page: createPageReducer(mockPageState)
  }))

  store.dispatch = jest.fn(store.dispatch)
  return store
}

function renderHookOptionsWithCustomStore (store: any) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) =>
      <Provider store={store}>
        <LibContext.Provider value={MockedLib as any}>
          {children}
        </LibContext.Provider>
      </Provider>
  }
}

describe('useAssetManagement hook', () => {
  it('should add an asset', async () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    const { wallet: { userVisibleTokensInfo } } = store.getState()
    act(
      () => result.current.onUpdateVisibleAssets(
        [...userVisibleTokensInfo, mockCustomToken])
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.addUserAsset({ ...mockCustomToken, logo: '' })
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should remove an asset', () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    const { wallet: { userVisibleTokensInfo } } = store.getState()
    act(
      () => result.current.onUpdateVisibleAssets(
        userVisibleTokensInfo.slice(1))
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.removeUserAsset(userVisibleTokensInfo[0])
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should remove and add an asset', () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    const { wallet: { userVisibleTokensInfo } } = store.getState()
    act(
      () => result.current.onUpdateVisibleAssets(
        [...userVisibleTokensInfo.slice(1), mockCustomToken])
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

  it('should set custom tokens visibility to false', () => {
    const customStore = createStore(combineReducers({
      wallet: createWalletReducer({
        ...mockWalletState,
        userVisibleTokensInfo: [
          mockCustomToken,
          ...mockWalletState.userVisibleTokensInfo
        ]
      }),
      page: createPageReducer(mockPageState)
    }))
    const store = makeStore(customStore)
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    const { wallet: { userVisibleTokensInfo } } = store.getState()
    act(
      () => result.current.onUpdateVisibleAssets(
        [{ ...mockCustomToken, visible: false }, ...userVisibleTokensInfo.slice(1)])
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

  it('should set custom tokens visibility to true', () => {
    const customStore = createStore(combineReducers({
      wallet: createWalletReducer({
        ...mockWalletState,
        userVisibleTokensInfo: [
          { ...mockCustomToken, visible: false },
          ...mockWalletState.userVisibleTokensInfo
        ]
      }),
      page: createPageReducer(mockPageState)
    }))
    const store = makeStore(customStore)
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    act(
      () => result.current.onUpdateVisibleAssets(
        [mockCustomToken, ...mockWalletState.userVisibleTokensInfo]
      )
    )
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.setUserAssetVisible({ token: mockCustomToken, isVisible: true })
    )
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
})

  it('should add token to visible assets list if not found', () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    act(
      () => result.current.makeTokenVisible(mockCustomToken)
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.addUserAsset({ ...mockCustomToken, logo: '' })
    )
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })

  it('should not add token to visible list if already there', () => {
    const store = makeStore()
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    const { wallet: { userVisibleTokensInfo } } = store.getState()
    act(
      () => result.current.makeTokenVisible(userVisibleTokensInfo[0])
    )

    expect(store.dispatch).not.toHaveBeenCalled()
  })

  it('should update visibility of token if not visible already', () => {
    const customStore = createStore(combineReducers({
      wallet: createWalletReducer({
        ...mockWalletState,
        userVisibleTokensInfo: [
          { ...mockCustomToken, visible: false },
          ...mockWalletState.userVisibleTokensInfo
        ]
      }),
      page: createPageReducer(mockPageState)
    }))
    const store = makeStore(customStore)
    const renderHookOptions = renderHookOptionsWithCustomStore(store)
    const { result } = renderHook(() => useAssetManagement(), renderHookOptions)

    act(
      () => result.current.makeTokenVisible(mockCustomToken)
    )

    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.setUserAssetVisible({ token: mockCustomToken, isVisible: true })
    )
    expect(store.dispatch).toHaveBeenCalledWith(
      WalletActions.refreshBalancesAndPriceHistory()
    )
  })
})
