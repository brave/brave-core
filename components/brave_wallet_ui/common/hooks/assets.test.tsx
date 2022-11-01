import * as React from 'react'
import { createStore, combineReducers } from 'redux'
import { Provider } from 'react-redux'
import { TextEncoder, TextDecoder } from 'util'
// @ts-expect-error
global.TextDecoder = TextDecoder
global.TextEncoder = TextEncoder
import { renderHook, act } from '@testing-library/react-hooks'
import {
  mockAccount,
  mockAssetPrices,
  mockNetwork
} from '../constants/mocks'
import useAssets from './assets'
import { WalletAccountType } from '../../constants/types'
import { setMockedBuyAssets } from '../async/__mocks__/lib'
import * as MockedLib from '../async/__mocks__/lib'
import { LibContext } from '../context/lib.context'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { createWalletReducer, createWalletSlice } from '../reducers/wallet_reducer'
import { mockBasicAttentionToken, mockEthToken } from '../../stories/mock-data/mock-asset-options'

const mockAccounts = [
  {
    ...mockAccount,
    tokenBalanceRegistry: {
      [mockEthToken.contractAddress.toLowerCase()]: '238699740940532500',
      [mockBasicAttentionToken.contractAddress.toLowerCase()]: '0'
    }
  } as WalletAccountType,
  {
    ...mockAccount,
    balance: '',
    tokenBalanceRegistry: {
      [mockEthToken.contractAddress.toLowerCase()]: '0',
      [mockBasicAttentionToken.contractAddress.toLowerCase()]: '0'
    }
  } as WalletAccountType
]

const mockVisibleList = [
  mockEthToken,
  mockBasicAttentionToken
]

const renderHookOptionsWithCustomStore = (store: any) => ({
  wrapper: ({ children }: { children?: React.ReactChildren }) =>
    <Provider store={store}>
      <LibContext.Provider value={MockedLib as any}>
        {children}
      </LibContext.Provider>
    </Provider>
})

describe('useAssets hook', () => {
  it('Selected account has balances, should return expectedResult', async () => {
    setMockedBuyAssets(mockVisibleList)
    const { result, waitForNextUpdate } = renderHook(
      () => useAssets(),
      renderHookOptionsWithCustomStore(
        createStore(combineReducers({
          wallet: createWalletSlice({
            ...mockWalletState,
            userVisibleTokensInfo: mockVisibleList,
            selectedAccount: mockAccounts[0],
            accounts: mockAccounts,
            transactionSpotPrices: mockAssetPrices,
            selectedNetwork: mockNetwork,
            networkList: [mockNetwork]
          }).reducer
        }))
      )
    )
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual(mockVisibleList)
  })

  it('should return empty array for panelUserAssetList if visible assets is empty', async () => {
    const { result, waitForNextUpdate } = renderHook(
      () => useAssets(),
      renderHookOptionsWithCustomStore(
        createStore(combineReducers({
          wallet: createWalletReducer({
            ...mockWalletState,
            userVisibleTokensInfo: [],
            selectedAccount: mockAccounts[0],
            accounts: mockAccounts,
            transactionSpotPrices: mockAssetPrices,
            selectedNetwork: mockNetwork,
            networkList: [mockNetwork]
          })
        }))
      )
    )
    await act(async () => {
      await waitForNextUpdate()
    })
    expect(result.current.panelUserAssetList).toEqual([])
  })
})
