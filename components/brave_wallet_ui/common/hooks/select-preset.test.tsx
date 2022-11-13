// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { Provider } from 'react-redux'
import { createStore, combineReducers } from 'redux'
import { renderHook } from '@testing-library/react-hooks'
// Mocks
import { mockPageState } from '../../stories/mock-data/mock-page-state'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { mockERC20Token, mockAccount } from '../constants/mocks'
// Reducers
import { createPageReducer } from '../../page/reducers/page_reducer'
import { createWalletReducer } from '../slices/wallet.slice'
// Hooks
import usePreset from './select-preset'

describe('usePreset hook', () => {
  it.each([
    ['25% of 1 ETH', '1000000000000000000', 0.25, '0.25'],
    ['50% of 1 ETH', '1000000000000000000', 0.50, '0.5'],
    ['75% of 1 ETH', '1000000000000000000', 0.75, '0.75'],
    ['100% of 1 ETH', '1000000000000000000', 1, '1'],
    ['100% of 1.000000000000000001 ETH', '1000000000000000001', 1, '1.000000000000000001'], // 1.000000000000000001 ---(100%)---> 1.000000000000000001
    ['100% of 50.297939 ETH', '50297939000000000000', 1, '50.297939'], // 50.297939 ---(100%)---> 50.297939
    ['25% of 0.0001 ETH', '100000000000000', 0.25, '0.000025'] // 0.0001 ---(25%)---> 0.000025
  ])('should compute %s correctly', (_, balance: string, percent, expected: string) => {
    const mockFunc = jest.fn()

    const store = createStore(combineReducers({
      wallet: createWalletReducer({
        ...mockWalletState,
        selectedAccount: { ...mockAccount, tokenBalanceRegistry: { [mockERC20Token.contractAddress.toLowerCase()]: balance } }
      }),
      page: createPageReducer(mockPageState)
    }))

    const { result: { current: calcPresetAmount } } = renderHook(() => usePreset(
      {
        onSetAmount: mockFunc,
        asset: mockERC20Token
      }
    ),
      {
        wrapper: ({ children }) => <Provider store={store}>{children}</Provider>
      })

    calcPresetAmount(percent)
    expect(mockFunc.mock.calls.length).toBe(1)
    expect(mockFunc.mock.calls[0][0]).toBe(expected)
  })

  it('should not do anything if send/swap asset is undefined', () => {
    const mockOnSetFromAmount = jest.fn()

    const store = createStore(combineReducers({
      wallet: createWalletReducer(mockWalletState),
      page: createPageReducer(mockPageState)
    }))

    const { result: { current: calcPresetAmount } } = renderHook(() => usePreset(
      {
        onSetAmount: mockOnSetFromAmount,
        asset: undefined
      }),
      {
        wrapper: ({ children }) => <Provider store={store}>{children}</Provider>
      })

    calcPresetAmount(0.25)
    expect(mockOnSetFromAmount.mock.calls.length).toBe(0)
  })
})
