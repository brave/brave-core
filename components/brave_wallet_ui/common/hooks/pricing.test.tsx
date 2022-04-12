// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createStore, combineReducers } from 'redux'
import { Provider } from 'react-redux'

import { renderHook } from '@testing-library/react-hooks'
import { createPageReducer } from '../../page/reducers/page_reducer'
import { mockPageState } from '../../stories/mock-data/mock-page-state'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'

import {
  mockAssetPrices
} from '../constants/mocks'
import { createWalletReducer } from '../reducers/wallet_reducer'
import usePricing from './pricing'

const makeStore = (customStore?: any) => {
  const store = customStore || createStore(combineReducers({
    wallet: createWalletReducer({
      ...mockWalletState,
      transactionSpotPrices: mockAssetPrices
    }),
    page: createPageReducer(mockPageState)
  }))

  store.dispatch = jest.fn(store.dispatch)
  return store
}

const store = makeStore()

function renderHookOptionsWithCustomStore (store: any) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) =>
      <Provider store={store}>
        {children}
      </Provider>
  }
}

describe('usePricing hook', () => {
  it('should return asset price of DOG token', () => {
    const { result } = renderHook(() => usePricing(), renderHookOptionsWithCustomStore(store))
    expect(result.current.findAssetPrice('DOG')).toEqual('100')
  })

  it('should return empty asset price of unknown token', () => {
    const { result } = renderHook(() => usePricing(), renderHookOptionsWithCustomStore(store))
    expect(result.current.findAssetPrice('CAT')).toEqual('')
  })

  it('should compute fiat amount for DOG token', () => {
    const { result } = renderHook(() => usePricing(), renderHookOptionsWithCustomStore(store))
    expect(result.current.computeFiatAmount('7', 'DOG', 1).formatAsFiat()).toEqual('70.00')
  })

  it('should return empty fiat value for unknown token', () => {
    const { result } = renderHook(() => usePricing(), renderHookOptionsWithCustomStore(store))
    expect(result.current.computeFiatAmount('7', 'CAT', 0).formatAsFiat()).toEqual('')
  })

  it('should return empty fiat value for empty amount', () => {
    const { result } = renderHook(() => usePricing(), renderHookOptionsWithCustomStore(store))
    expect(result.current.computeFiatAmount('', 'DOG', 0).formatAsFiat()).toEqual('')
  })
})
